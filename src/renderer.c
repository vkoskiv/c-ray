//
//  renderer.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "renderer.h"

/*
 * Global renderer
 */
renderer mainRenderer;

#ifdef WINDOWS
HANDLE tileMutex = INVALID_HANDLE_VALUE;
#else
pthread_mutex_t tileMutex;
#endif

#pragma mark Helper funcs

bool renderTilesEmpty() {
	return mainRenderer.renderedTileCount >= mainRenderer.tileCount;
}

/**
 Gets the next tile from renderTiles in mainRenderer
 
 @return A renderTile to be rendered
 */
renderTile getTile() {
#ifdef WINDOWS
	WaitForSingleObject(tileMutex, INFINITE);
#else
	pthread_mutex_lock(&tileMutex);
#endif
	//FIXME: This could be optimized
	renderTile tile = mainRenderer.renderTiles[mainRenderer.renderedTileCount++];
	mainRenderer.renderTiles[mainRenderer.renderedTileCount - 1].isRendering = true;
	tile.tileNum = mainRenderer.renderedTileCount - 1;
	
#ifdef WINDOWS
	ReleaseMutex(tileMutex);
#else
	pthread_mutex_unlock(&tileMutex);
#endif
	return tile;
}

/**
 Create tiles from render plane, and add those to mainRenderer
 
 @param worldScene worldScene object
 */
void quantizeImage(scene *worldScene) {
#ifdef WINDOWS
	//Create this here for now
	tileMutex = CreateMutex(NULL, FALSE, NULL);
#endif
	printf("Quantizing render plane...\n");
	int tilesX = worldScene->camera->width / worldScene->camera->tileWidth;
	int tilesY = worldScene->camera->height / worldScene->camera->tileHeight;
	
	float tilesXf = (float)worldScene->camera->width / (float)worldScene->camera->tileWidth;
	float tilesYf = (float)worldScene->camera->height / (float)worldScene->camera->tileHeight;
	
	if (tilesXf - (int)tilesXf != 0) {
		tilesX++;
	}
	if (tilesYf - (int)tilesYf != 0) {
		tilesY++;
	}
	
	mainRenderer.renderTiles = (renderTile*)calloc(tilesX*tilesY, sizeof(renderTile));
	
	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			renderTile *tile = &mainRenderer.renderTiles[x + y*tilesX];
			tile->width  = worldScene->camera->tileWidth;
			tile->height = worldScene->camera->tileHeight;
			
			tile->startX = x       * worldScene->camera->tileWidth;
			tile->endX   = (x + 1) * worldScene->camera->tileWidth;
			
			tile->startY = y       * worldScene->camera->tileHeight;
			tile->endY   = (y + 1) * worldScene->camera->tileHeight;
			
			tile->endX = min((x + 1) * worldScene->camera->tileWidth, worldScene->camera->width);
			tile->endY = min((y + 1) * worldScene->camera->tileHeight, worldScene->camera->height);
			
			tile->completedSamples = 1;
			tile->isRendering = false;
			
			mainRenderer.tileCount++;
		}
	}
	printf("Quantized image into %i tiles. (%ix%i)", (tilesX*tilesY), tilesX, tilesY);
}

void reorderTopToBottom() {
	int endIndex = mainRenderer.tileCount - 1;
	
	renderTile *tempArray = (renderTile*)calloc(mainRenderer.tileCount, sizeof(renderTile));
	
	for (int i = 0; i < mainRenderer.tileCount; i++) {
		tempArray[i] = mainRenderer.renderTiles[endIndex--];
	}
	
	free(mainRenderer.renderTiles);
	mainRenderer.renderTiles = tempArray;
}

void reorderFromMiddle() {
	int midLeft = 0;
	int midRight = 0;
	bool isRight = true;
	
	midRight = ceil(mainRenderer.tileCount / 2);
	midLeft = midRight - 1;
	
	renderTile *tempArray = (renderTile*)calloc(mainRenderer.tileCount, sizeof(renderTile));
	
	for (int i = 0; i < mainRenderer.tileCount; i++) {
		if (isRight) {
			tempArray[i] = mainRenderer.renderTiles[midRight++];
			isRight = false;
		} else {
			tempArray[i] = mainRenderer.renderTiles[midLeft--];
			isRight = true;
		}
	}
	
	free(mainRenderer.renderTiles);
	mainRenderer.renderTiles = tempArray;
}

/**
 Reorder renderTiles in given order
 
 @param order Render order to be applied
 */
void reorderTiles(renderOrder order) {
	switch (order) {
		case renderOrderFromMiddle:
		{
			reorderFromMiddle();
		}
			break;
		case renderOrderTopToBottom:
		{
			reorderTopToBottom();
		}
			break;
		default:
			break;
	}
}

/**
 Gets a pixel from the render buffer
 
 @param worldScene WorldScene to get image dimensions
 @param x X coordinate of pixel
 @param y Y coordinate of pixel
 @return A color object, with full color precision intact (double)
 */
color getPixel(scene *worldScene, int x, int y) {
	color output = {0.0f, 0.0f, 0.0f, 0.0f};
	output.red =   mainRenderer.renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 0];
	output.green = mainRenderer.renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 1];
	output.blue =  mainRenderer.renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 2];
	output.alpha = 1.0f;
	return output;
}

/**
 Returns a random float between min and max
 
 @param min Minimum value
 @param max Maximum value
 @return Random float between min and max
 */
float getRandomFloat(float min, float max) {
	return ((((float)rand()) / (float)RAND_MAX) * (max - min)) + min;
}


/**
 Returns a randomized position in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position within a radius of center point
 */
vector getRandomVecOnRadius(vector center, float radius) {
	return vectorWithPos(center.x + getRandomFloat(-radius, radius),
						 center.y + getRandomFloat(-radius, radius),
						 center.z + getRandomFloat(-radius, radius));
}

/**
 Returns a randomized position on a plane in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position on a plane within a radius of center point
 */
vector getRandomVecOnPlane(vector center, float radius) {
	//FIXME: This only works in one orientation!
	return vectorWithPos(center.x + getRandomFloat(-radius, radius),
						 center.y + getRandomFloat(-radius, radius),
						 center.z);
}

#pragma mark Renderer

/*color rayTrace2(lightRay *incidentRay, world *worldScene) {
	color output = {0.0f,0.0f,0.0f};
	int bounces = 0;
	double contrast = worldScene->camera->contrast;
	vector hitpoint, hitpointNormal;
	
	do{
 // Find closest intersection
 double closestIntersection = 20000.0f;
 int currentSphere = -1;
 int currentPolygon = -1;
 int sphereAmount = worldScene->sphereAmount;
 int polygonAmount = fullPolyCount;
 int lightSourceAmount = worldScene->lightAmount;
 int objCount = worldScene->objCount;
 
 material currentMaterial;
 vector polyNormal = {0.0, 0.0, 0.0};
 
 //Sphere handling
 unsigned int i;
 for(i = 0; i < sphereAmount; i++){
 if(rayIntersectsWithSphere(incidentRay, &worldScene->spheres[i], &closestIntersection))
 currentSphere = i;
 }
 
 //OBJ handling
 double fakeIntersection = 20000.0f;
 unsigned int o, p;
 for (o = 0; o < objCount; o++) {
 if (rayIntersectsWithSphere(incidentRay, &worldScene->objs[o].boundingVolume, &fakeIntersection)) {
 for (p = worldScene->objs[0].firstPolyIndex; p < (worldScene->objs[o].firstPolyIndex + worldScene->objs[0].polyCount); p++) {
 if (rayIntersectsWithPolygon(incidentRay, &polygonArray[p], &closestIntersection, &polyNormal)) {
 currentPolygon = p;
 currentSphere = -1;
 }
 }
 }
 }
 
 if(currentSphere != -1) {
 vector scaled = vectorScale(closestIntersection, &incidentRay->direction);
 hitpoint = addVectors(&incidentRay->start, &scaled);
 
 // Find the normal for this new vector at the point of intersection
 hitpointNormal = subtractVectors(&hitpoint, &worldScene->spheres[currentSphere].pos);
 float temp = scalarProduct(&hitpointNormal, &hitpointNormal);
 if(temp == 0) break;
 
 temp = 1.0f / sqrtf(temp);
 hitpointNormal = vectorScale(temp, &hitpointNormal);
 
 // Find the material to determine the colour
 currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].material];
 } else if (currentPolygon != -1) {
 vector scaled = vectorScale(closestIntersection, &incidentRay->direction);
 hitpoint = addVectors(&incidentRay->start, &scaled);
 //FIXME: Get this from OBJ?
 hitpointNormal = polyNormal;
 float temp = scalarProduct(&hitpointNormal, &hitpointNormal);
 if (temp == 0) break;
 temp = 1.0f / sqrtf(temp);
 hitpointNormal = vectorScale(temp, &hitpointNormal);
 currentMaterial = worldScene->materials[polygonArray[currentPolygon].materialIndex];
 } else {
 //Ray missed everything, set to background.
 color temp = colorCoef(contrast, worldScene->ambientColor);
 output = addColors(&output, &temp);
 break;
 }
 
 // Find the value of the light at this point
 unsigned int j;
 for(j=0; j < 3; j++){
 light currentLight = worldScene->lights[j];
 vector dist = subtractVectors(&currentLight.pos, &hitpoint);
 if(scalarProduct(&hitpointNormal, &dist) <= 0.0f) continue;
 double t = sqrtf(scalarProduct(&dist,&dist));
 if(t <= 0.0f) continue;
 
 lightRay bouncedRay;
 bouncedRay.start = hitpoint;
 bouncedRay.direction = vectorScale((1/t), &dist);
 
 // Calculate shadows
 bool inShadow = false;
 unsigned int k;
 for (k = 0; k < 3; ++k) {
 if (rayIntersectsWithSphere(&bouncedRay, &worldScene->spheres[k], &t)){
 inShadow = true;
 break;
 }
 }
 if (!inShadow){
 // Lambert diffusion
 float lambert = scalarProduct(&bouncedRay.direction, &hitpointNormal) * contrast;
 output.red += lambert * currentLight.intensity.red * currentMaterial.diffuse.red;
 output.green += lambert * currentLight.intensity.green * currentMaterial.diffuse.green;
 output.blue += lambert * currentLight.intensity.blue * currentMaterial.diffuse.blue;
 }
 }
 // Iterate over the reflection
 contrast *= currentMaterial.reflectivity;
 
 // The reflected ray start and direction
 incidentRay->start = hitpoint;
 float reflect = 2.0f * scalarProduct(&incidentRay->direction, &hitpointNormal);
 vector tmp = vectorScale(reflect, &hitpointNormal);
 incidentRay->direction = subtractVectors(&incidentRay->direction, &tmp);
 
 bounces++;
 
	} while((contrast > 0.0f) && (bounces < 15));
	return output;
 }*/

/**
 Returns a computed color based on a given ray and world scene
 
 @param incidentRay View ray to be cast into a scene
 @param worldScene Scene the ray is cast into
 @return Color value with full precision (double)
 */
color rayTrace(lightRay *incidentRay, scene *worldScene) {
	//Raytrace a given light ray with a given scene, then return the color value for that ray
	color output = {0.0f,0.0f,0.0f};
	int bounces = 0;
	double contrast = worldScene->camera->contrast;
	
	do {
		//Find the closest intersection first
		double closestIntersection = 20000.0f;
		double temp;
		int currentSphere = -1;
		int currentPolygon = -1;
		int sphereAmount = worldScene->sphereCount;
		int lightSourceAmount = worldScene->lightCount;
		int objCount = worldScene->objCount;
		
		material currentMaterial;
		vector polyNormal = {0.0, 0.0, 0.0};
		vector hitpoint, surfaceNormal;
		
		bool isCustomPoly = false;
		
		unsigned int i;
		for (i = 0; i < sphereAmount; ++i) {
			if (rayIntersectsWithSphere(incidentRay, &worldScene->spheres[i], &closestIntersection)) {
				currentSphere = i;
			}
		}
		
		double fakeIntersection = 20000.0f;
		unsigned int o, p;
		for (o = 0; o < objCount; o++) {
			if (rayIntersectsWithSphere(incidentRay, &worldScene->objs[o].boundingVolume, &fakeIntersection)) {
				for (p = worldScene->objs[o].firstPolyIndex; p < (worldScene->objs[o].firstPolyIndex + worldScene->objs[o].polyCount); p++) {
					if (rayIntersectsWithPolygon(incidentRay, &polygonArray[p], &closestIntersection, &polyNormal)) {
						currentPolygon = p;
						currentMaterial = *worldScene->objs[o].material;
						currentSphere = -1;
						isCustomPoly = false;
					}
				}
			}
		}
		
		//FIXME: TEMPORARY
		for (i = 0; i < worldScene->customPolyCount; ++i) {
			if (rayIntersectsWithPolygon(incidentRay, &worldScene->customPolys[i], &closestIntersection, &polyNormal)) {
				currentPolygon = i;
				currentSphere = -1;
				isCustomPoly = true;
			}
		}
		
		//Ray-object intersection detection
		if (currentSphere != -1) {
			vector scaled = vectorScale(closestIntersection, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			surfaceNormal = subtractVectors(&hitpoint, &worldScene->spheres[currentSphere].pos);
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			surfaceNormal = vectorScale(temp, &surfaceNormal);
			currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].materialIndex];
		} else if (currentPolygon != -1) {
			vector scaled = vectorScale(closestIntersection, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			//We get polyNormal from the intersection function
			surfaceNormal = polyNormal;
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			//FIXME: Possibly get existing normal here
			surfaceNormal = vectorScale(temp, &surfaceNormal);
			//FIXME: TEMPORARY
			if (isCustomPoly) {
				currentMaterial = worldScene->materials[worldScene->customPolys[currentPolygon].materialIndex];
			}
		} else {
			//Ray didn't hit any object, set color to ambient
			color temp = colorCoef(contrast, worldScene->ambientColor);
			output = addColors(&output, &temp);
			break;
		}
		
		if (scalarProduct(&surfaceNormal, &incidentRay->direction) < 0.0f) {
			surfaceNormal = vectorScale(1.0f, &surfaceNormal);
		} else if (scalarProduct(&surfaceNormal, &incidentRay->direction) > 0.0f) {
			surfaceNormal = vectorScale(-1.0f, &surfaceNormal);
		}
		
		lightRay bouncedRay, cameraRay;
		bouncedRay.start = hitpoint;
		cameraRay.start = hitpoint;
		cameraRay.direction = subtractVectors(&worldScene->camera->pos, &hitpoint);
		double cameraProjection = scalarProduct(&cameraRay.direction, &hitpoint);
		double cameraDistance = scalarProduct(&cameraRay.direction, &cameraRay.direction);
		double camTemp = cameraDistance;
		camTemp = invsqrtf(camTemp);
		cameraRay.direction = vectorScale(camTemp, &cameraRay.direction);
		cameraProjection = camTemp * cameraProjection;
		//Find the value of the light at this point
		unsigned int j;
		for (j = 0; j < lightSourceAmount; ++j) {
			light currentLight = worldScene->lights[j];
			vector lightPos;
			if (worldScene->camera->areaLights)
				lightPos = getRandomVecOnRadius(currentLight.pos, currentLight.radius);
			else
				lightPos = currentLight.pos;
			
			bouncedRay.direction = subtractVectors(&lightPos, &hitpoint);
			
			double lightProjection = scalarProduct(&bouncedRay.direction, &surfaceNormal);
			if (lightProjection <= 0.0f) continue;
			
			double lightDistance = scalarProduct(&bouncedRay.direction, &bouncedRay.direction);
			double temp = lightDistance;
			
			if (temp <= 0.0f) continue;
			temp = invsqrtf(temp);
			bouncedRay.direction = vectorScale(temp, &bouncedRay.direction);
			lightProjection = temp * lightProjection;
			
			//Calculate shadows
			bool inShadow = false;
			double t = lightDistance;
			unsigned int k;
			for (k = 0; k < sphereAmount; ++k) {
				if (rayIntersectsWithSphere(&bouncedRay, &worldScene->spheres[k], &t)) {
					inShadow = true;
					break;
				}
			}
			
			double fakeIntersection = 20000.0f;
			for (o = 0; o < objCount; o++) {
				if (rayIntersectsWithSphere(&bouncedRay, &worldScene->objs[o].boundingVolume, &fakeIntersection)) {
					
					if (worldScene->camera->aprxShadows) {
						inShadow = true;
						break;
					} else {
						for (p = worldScene->objs[o].firstPolyIndex; p < (worldScene->objs[o].firstPolyIndex + worldScene->objs[o].polyCount); p++) {
							if (rayIntersectsWithPolygon(&bouncedRay, &polygonArray[p], &t, &polyNormal)) {
								inShadow = true;
								break;
							}
						}
					}
				}
			}
			
			//FIXME: TEMPORARY
			for (i = 0; i < worldScene->customPolyCount; ++i) {
				if (rayIntersectsWithPolygon(&bouncedRay, &worldScene->customPolys[i], &t, &polyNormal)) {
					inShadow = true;
					break;
				}
			}
			
			if (!inShadow) {
				//TODO: Calculate specular reflection
				float specularFactor = 1.0;//scalarProduct(&cameraRay.direction, &surfaceNormal) * contrast;
				
				//Calculate Lambert diffusion
				float diffuseFactor = scalarProduct(&bouncedRay.direction, &surfaceNormal) * contrast;
				output.red += specularFactor * diffuseFactor * currentLight.intensity.red * currentMaterial.diffuse.red;
				output.green += specularFactor * diffuseFactor * currentLight.intensity.green * currentMaterial.diffuse.green;
				output.blue += specularFactor * diffuseFactor * currentLight.intensity.blue * currentMaterial.diffuse.blue;
			}
		}
		//Iterate over the reflection
		contrast *= currentMaterial.reflectivity;
		
		//Calculate reflected ray start and direction
		double reflect = 2.0f * scalarProduct(&incidentRay->direction, &surfaceNormal);
		incidentRay->start = hitpoint;
		vector tempVec = vectorScale(reflect, &surfaceNormal);
		incidentRay->direction = subtractVectors(&incidentRay->direction, &tempVec);
		
		bounces++;
		
	} while ((contrast > 0.0f) && (bounces <= worldScene->camera->bounces));
	
	return output;
}

//Compute view direction transforms
void transformCameraView(vector *direction) {
	for (int i = 1; i < mainRenderer.worldScene->camTransformCount; i++) {
		transformVector(direction, &mainRenderer.worldScene->camTransforms[i]);
		direction->isTransformed = false;
	}
}

/**
 A render thread
 
 @param arg Thread information (see threadInfo struct)
 @return Exits when thread is done
 */
#ifdef WINDOWS
DWORD WINAPI renderThread(LPVOID arg) {
#else
	void *renderThread(void *arg) {
#endif
		lightRay incidentRay;
		int x,y;
		
		threadInfo *tinfo = (threadInfo*)arg;
		
		renderTile tile;
		tile.tileNum = 0;
		while (!renderTilesEmpty()) {
			x = 0; y = 0;
			//FIXME: First tile on first thread doesn't show frame, probably because of this.
			mainRenderer.renderTiles[tile.tileNum].isRendering = false;
			tile = getTile();
			printf("Started tile %i/%i\r", mainRenderer.renderedTileCount, mainRenderer.tileCount);
			while (tile.completedSamples < mainRenderer.worldScene->camera->sampleCount+1 && mainRenderer.isRendering) {
				for (y = tile.endY; y > tile.startY; y--) {
					for (x = tile.startX; x < tile.endX; x++) {
						color output = getPixel(mainRenderer.worldScene, x, y);
						color sample = {0.0f,0.0f,0.0f,0.0f};
						
						int height = mainRenderer.worldScene->camera->height;
						int width = mainRenderer.worldScene->camera->width;
						
						double focalLength = 0.0f;
						if (mainRenderer.worldScene->camera->FOV > 0.0f
							&& mainRenderer.worldScene->camera->FOV < 189.0f) {
							focalLength = 0.5f * mainRenderer.worldScene->camera->width / tanf((double)(PIOVER180) * 0.5f * mainRenderer.worldScene->camera->FOV);
						}
						
						vector direction = {(x - 0.5f * mainRenderer.worldScene->camera->width) / focalLength,
							(y - 0.5f * mainRenderer.worldScene->camera->height) / focalLength, 1.0f};
						
						direction = normalizeVector(&direction);
						vector startPos = mainRenderer.worldScene->camera->pos;
						
						//And now compute transforms for position
						transformVector(&startPos, &mainRenderer.worldScene->camTransforms[0]);
						//...and compute rotation transforms for camera orientation
						transformCameraView(&direction);
						//Easy!
						
						//Set up ray
						incidentRay.start = startPos;
						incidentRay.direction = direction;
						incidentRay.rayType = rayTypeIncident;
						//Get sample
						sample = rayTrace(&incidentRay, mainRenderer.worldScene);
						
						//And process the running average
						output.red = output.red * (tile.completedSamples - 1);
						output.green = output.green * (tile.completedSamples - 1);
						output.blue = output.blue * (tile.completedSamples - 1);
						
						output = addColors(&output, &sample);
						
						output.red = output.red / tile.completedSamples;
						output.green = output.green / tile.completedSamples;
						output.blue = output.blue / tile.completedSamples;
						
						//Store render buffer
						mainRenderer.renderBuffer[(x + (height - y)*width)*3 + 0] = output.red;
						mainRenderer.renderBuffer[(x + (height - y)*width)*3 + 1] = output.green;
						mainRenderer.renderBuffer[(x + (height - y)*width)*3 + 2] = output.blue;
						
						//And store the image data
						mainRenderer.worldScene->camera->imgData[(x + (height - y)*width)*3 + 0] = (unsigned char)min(  output.red*255.0f, 255.0f);
						mainRenderer.worldScene->camera->imgData[(x + (height - y)*width)*3 + 1] = (unsigned char)min(output.green*255.0f, 255.0f);
						mainRenderer.worldScene->camera->imgData[(x + (height - y)*width)*3 + 2] = (unsigned char)min( output.blue*255.0f, 255.0f);
					}
				}
				tile.completedSamples++;
			}
			
		}
		mainRenderer.renderTiles[tile.tileNum].isRendering = false;
		printf("Thread %i done\n", tinfo->thread_num);
		tinfo->threadComplete = true;
#ifdef WINDOWS
		//Return possible codes here
		return 0;
#else
		pthread_exit((void*) arg);
#endif
}
