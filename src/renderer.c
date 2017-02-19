//
//  renderer.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "renderer.h"

#pragma mark Helper funcs
#define let int

renderer mainRenderer;

color getPixel(world *worldScene, int x, int y) {
	color output = {0.0f, 0.0f, 0.0f, 0.0f};
	output.red = mainRenderer.renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 0];
	output.green = mainRenderer.renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 1];
	output.blue = mainRenderer.renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 2];
	output.alpha = 1.0f;
	return output;
}

float getRandomFloat(float min, float max) {
	return ((((float)rand()) / (float)RAND_MAX) * (max - min)) + min;
}

vector getRandomVecOnRadius(vector center, float radius) {
	float x, y, z;
	x = getRandomFloat(-radius, radius);
	y = getRandomFloat(-radius, radius);
	z = getRandomFloat(-radius, radius);
	return vectorWithPos(center.x + x, center.y + y, center.z + z);
}

#pragma mark Renderer

color rayTrace(lightRay *incidentRay, world *worldScene) {
	//Raytrace a given light ray with a given scene, then return the color value for that ray
	color output = {0.0f,0.0f,0.0f};
	int bounces = 0;
	double contrast = worldScene->camera->contrast;
	
	do {
		//Find the closest intersection first
		double t = 20000.0f;
		double temp;
		int currentSphere = -1;
		int currentPolygon = -1;
		int sphereAmount = worldScene->sphereAmount;
		int polygonAmount = fullPolyCount;
		int lightSourceAmount = worldScene->lightAmount;
		
		material currentMaterial;
		vector polyNormal = {0.0, 0.0, 0.0};
		vector hitpoint, surfaceNormal;
		
		unsigned int i;
		for (i = 0; i < sphereAmount; ++i) {
			if (rayIntersectsWithSphere(incidentRay, &worldScene->spheres[i], &t)) {
				currentSphere = i;
			}
		}
		
		for (i = 0; i < polygonAmount; ++i) {
			if (rayIntersectsWithPolygon(incidentRay, &polygonArray[i], &t, &polyNormal)) {
				currentPolygon = i;
				currentSphere = -1;
			}
		}
		
		//Ray-object intersection detection
		if (currentSphere != -1) {
			vector scaled = vectorScale(t, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			surfaceNormal = subtractVectors(&hitpoint, &worldScene->spheres[currentSphere].pos);
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			surfaceNormal = vectorScale(temp, &surfaceNormal);
			currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].material];
		} else if (currentPolygon != -1) {
			vector scaled = vectorScale(t, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			surfaceNormal = polyNormal;
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			surfaceNormal = vectorScale(temp, &surfaceNormal);
			currentMaterial = worldScene->materials[polygonArray[currentPolygon].materialIndex];
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
			
			if (temp == 0.0f) continue;
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
			
			for (k = 0; k < polygonAmount; ++k) {
				if (rayIntersectsWithPolygon(&bouncedRay, &polygonArray[k], &t, &polyNormal)) {
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

void *renderThread(void *arg) {
	lightRay incidentRay;
	int x,y;
	
	threadInfo *tinfo = (threadInfo*)arg;
	//Figure out which part to render based on current thread ID
	int limits[] = {(tinfo->thread_num * mainRenderer.sectionSize), (tinfo->thread_num * mainRenderer.sectionSize) + mainRenderer.sectionSize};
	int completedSamples = 1;
	
	while (completedSamples < mainRenderer.worldScene->camera->sampleCount+1 && mainRenderer.isRendering) {
		tinfo->completedSamples = completedSamples;
		for (y = limits[1]; y > limits[0]; y--) {
			for (x = 0; x < mainRenderer.worldScene->camera->width; x++) {
				color output = getPixel(mainRenderer.worldScene, x, y);
				color sample = {0.0f,0.0f,0.0f,0.0f};
				if (mainRenderer.worldScene->camera->viewPerspective.projectionType == ortho) {
					incidentRay.start.x = x/2;
					incidentRay.start.y = y/2;
					incidentRay.start.z = mainRenderer.worldScene->camera->pos.z;
					
					incidentRay.direction.x = 0;
					incidentRay.direction.y = 0;
					incidentRay.direction.z = 1;
					sample = rayTrace(&incidentRay, mainRenderer.worldScene);
				} else if (mainRenderer.worldScene->camera->viewPerspective.projectionType == conic) {
					double focalLength = 0.0f;
					if ((mainRenderer.worldScene->camera->viewPerspective.projectionType == conic)
						&& mainRenderer.worldScene->camera->viewPerspective.FOV > 0.0f
						&& mainRenderer.worldScene->camera->viewPerspective.FOV < 189.0f) {
						focalLength = 0.5f * mainRenderer.worldScene->camera->width / tanf((double)(PIOVER180) * 0.5f * mainRenderer.worldScene->camera->viewPerspective.FOV);
					}
					
					vector direction = {(x - 0.5f * mainRenderer.worldScene->camera->width) / focalLength,
						(y - 0.5f * mainRenderer.worldScene->camera->height) / focalLength, 1.0f};
					
					double normal = scalarProduct(&direction, &direction);
					if (normal == 0.0f)
					break;
					direction = vectorScale(invsqrtf(normal), &direction);
					vector startPos = mainRenderer.worldScene->camera->pos;
					
					incidentRay.start = startPos;
					incidentRay.direction = direction;
					sample = rayTrace(&incidentRay, mainRenderer.worldScene);
					
					output.red = output.red * (completedSamples - 1);
					output.green = output.green * (completedSamples - 1);
					output.blue = output.blue * (completedSamples - 1);
					
					output = addColors(&output, &sample);
					
					output.red = output.red / completedSamples;
					output.green = output.green / completedSamples;
					output.blue = output.blue / completedSamples;
					
					//Store render buffer
					mainRenderer.renderBuffer[(x + (mainRenderer.worldScene->camera->height - y)*mainRenderer.worldScene->camera->width)*3 + 0] = output.red;
					mainRenderer.renderBuffer[(x + (mainRenderer.worldScene->camera->height - y)*mainRenderer.worldScene->camera->width)*3 + 1] = output.green;
					mainRenderer.renderBuffer[(x + (mainRenderer.worldScene->camera->height - y)*mainRenderer.worldScene->camera->width)*3 + 2] = output.blue;
				}
				
				mainRenderer.worldScene->camera->imgData[(x + (mainRenderer.worldScene->camera->height - y)*mainRenderer.worldScene->camera->width)*3 + 0] = (unsigned char)min(  output.red*255.0f, 255.0f);
				mainRenderer.worldScene->camera->imgData[(x + (mainRenderer.worldScene->camera->height - y)*mainRenderer.worldScene->camera->width)*3 + 1] = (unsigned char)min(output.green*255.0f, 255.0f);
				mainRenderer.worldScene->camera->imgData[(x + (mainRenderer.worldScene->camera->height - y)*mainRenderer.worldScene->camera->width)*3 + 2] = (unsigned char)min( output.blue*255.0f, 255.0f);
			}
		}
		completedSamples++;
	}
	printf("Thread %i done\n", tinfo->thread_num);
	tinfo->threadComplete = true;
	pthread_exit((void*) arg);
}
