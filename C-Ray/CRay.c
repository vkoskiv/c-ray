//
//  CRay.c
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//
/*
 TODO:
 Add input file
 Add ambient color
 Add camera object
 */

#include "CRay.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define invsqrtf(x) (1.0f / sqrtf(x))

//Global variables
unsigned char *imgData = NULL;
unsigned long bounceCount = 0;

int main(int argc, char *argv[]) {
	
	time_t start, stop;
	
	//Seed RNGs
	srand((int)time(NULL));
	srand48(time(NULL));
	
	//Animation
	for (int currentFrame = 0; currentFrame < kFrameCount; currentFrame++) {
		//This is a timer to elapse how long a render takes
		time(&start);
		printf("Rendering at %i x %i\n",kImgWidth,kImgHeight);
		printf("Building scene\n");
		//Define the scene. Eventually read this from an input file
		lightRay incidentRay;
		
		//Create the scene
		world sceneObject;
		sceneObject.materials = NULL;
		sceneObject.spheres = NULL;
		sceneObject.polys = NULL;
		sceneObject.lights = NULL;
		world *worldScene = NULL;
		worldScene = &sceneObject;
		//Build the scene
		if (buildScene(false, worldScene) == -1) {
			printf("Error building worldScene\n");
		}
		
		printf("Using %i light bounces\n",bounces);
		printf("Raytracing...\n");
		//Create array of pixels for image data
		//Allocate memory for it, remember to free!
		if (imgData) {
			free(imgData);
		}
		imgData = (unsigned char*)malloc(3*kImgWidth*kImgHeight);
		memset(imgData, 0, 3*kImgWidth*kImgHeight);
		
		//Start filling array with image data with ray tracing
		int x, y;
		
		for (y = 0; y < kImgHeight; y++) {
			for (x = 0; x < kImgWidth; x++) {
				
				color output = {0.0f, 0.0f, 0.0f};
				
				int level = 0;
				double coefficient = contrast;
				
				//Camera position (Kind of, no real camera yet)
				incidentRay.start.x = x;
				incidentRay.start.y = y ;
				incidentRay.start.z = -2000;
				
				incidentRay.direction.x = 0;
				incidentRay.direction.y = 0;
				incidentRay.direction.z = 1;
				
				do {
					
					//Find the closest intersection first
					double t = 20000.0f;
					double temp;
					int currentSphere = -1;
					int currentPolygon = -1;
					int sphereAmount = worldScene->sphereAmount;
					int polygonAmount = worldScene->polygonAmount;
					int lightSourceAmount = worldScene->lightAmount;
					
					material currentMaterial;
					vector polyNormal, hitpoint, surfaceNormal;
					
					unsigned int i;
					for (i = 0; i < sphereAmount; i++) {
						if (rayIntersectsWithSphere(&incidentRay, &worldScene->spheres[i], &t)) {
							currentSphere = i;
						}
					}
					
					for (i = 0; i < polygonAmount; i++) {
						if (rayIntersectsWithPolygon(&incidentRay, &worldScene->polys[i], &t, &polyNormal)) {
							currentPolygon = i;
							currentSphere = -1;
						}
					}
					
					//Ray-object intersection detection
					if (currentSphere != -1) {
						bounceCount++;
						vector scaled = vectorScale(t, &incidentRay.direction);
						hitpoint = addVectors(&incidentRay.start, &scaled);
						surfaceNormal = subtractVectors(&hitpoint, &worldScene->spheres[currentSphere].pos);
						temp = scalarProduct(&surfaceNormal,&surfaceNormal);
						if (temp == 0.0f) break;
						temp = invsqrtf(temp);
						surfaceNormal = vectorScale(temp, &surfaceNormal);
						currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].material];
					} else if (currentPolygon != -1) {
						bounceCount++;
						vector scaled = vectorScale(t, &incidentRay.direction);
						hitpoint = addVectors(&incidentRay.start, &scaled);
						surfaceNormal = polyNormal;
						temp = scalarProduct(&surfaceNormal,&surfaceNormal);
						if (temp == 0.0f) break;
						temp = invsqrtf(temp);
						surfaceNormal = vectorScale(temp, &surfaceNormal);
						currentMaterial = worldScene->materials[worldScene->polys[currentPolygon].material];
					} else {
						//Ray didn't hit any object, set color to ambient
						color back;
						back.red = worldScene->ambientColor->red;
						back.green = worldScene->ambientColor->green;
						back.blue = worldScene->ambientColor->blue;
						color temp = colorCoef(coefficient, &back);
						output = addColors(&output, &temp);
						break;
					}
					
					bool isInside;
					
					if (scalarProduct(&surfaceNormal, &incidentRay.direction) > 0.0f) {
						surfaceNormal = vectorScale(-1.0f,&surfaceNormal);
						isInside = true;
					} else {
						isInside = false;
					}

					if (!isInside) {
						
						lightRay bouncedRay;
						bouncedRay.start = hitpoint;
						//Find the value of the light at this point (Scary!)
						unsigned int j;
						for (j = 0; j < lightSourceAmount; j++) {
							//TODO: Implement rayIntersectsWithSphere here
							lightSource currentLight = worldScene->lights[j];
							bouncedRay.direction = subtractVectors(&currentLight.pos, &hitpoint);
							
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
								if (rayIntersectsWithPolygon(&bouncedRay, &worldScene->polys[k], &t, &polyNormal)) {
									inShadow = true;
									break;
								}
							}
							if (!inShadow) {
								//Calculate Lambert diffusion
								float lambert = scalarProduct(&bouncedRay.direction, &surfaceNormal) * coefficient;
								output.red += lambert * currentLight.intensity.red * currentMaterial.diffuse.red;
								output.green += lambert * currentLight.intensity.green * currentMaterial.diffuse.green;
								output.blue += lambert * currentLight.intensity.blue * currentMaterial.diffuse.blue;
							}
						}
						//Iterate over the reflection
						coefficient *= currentMaterial.reflectivity;
						
						//Calculate reflected ray start and direction
						double reflect = 2.0f * scalarProduct(&incidentRay.direction, &surfaceNormal);
						incidentRay.start = hitpoint;
						vector temp = vectorScale(reflect, &surfaceNormal);
						incidentRay.direction = subtractVectors(&incidentRay.direction, &temp);
					}
					
					level++;
					
				} while ((coefficient > 0.0f) && (level < bounces));
				
				//Write pixel color channels to array
				imgData[(x + y*kImgWidth)*3 + 2] = (unsigned char)min(  output.red*255.0f, 255.0f);
				imgData[(x + y*kImgWidth)*3 + 1] = (unsigned char)min(output.green*255.0f, 255.0f);
				imgData[(x + y*kImgWidth)*3 + 0] = (unsigned char)min( output.blue*255.0f, 255.0f);
			}
		}
		printf("%lu light bounces\n",bounceCount);
		printf("Saving result\n");
		
		//Save image data to a file
		//String manipulation is lovely in C
		char buf[16];
		sprintf(buf, "rendered_%d.bmp", currentFrame);
		printf("%s\n", buf);
		saveBmpFromArray(buf, imgData, kImgWidth, kImgHeight);
		long bytes = 3*kImgWidth*kImgHeight;
		long mBytes = (bytes / 1024) / 1024;
		printf("Wrote %ld megabytes to file.\n",mBytes);
		
		//Free memory
		if (imgData)
			free(imgData);
		if (worldScene->lights)
			free(worldScene->lights);
		if (worldScene->spheres)
			free(worldScene->spheres);
		if (worldScene->polys)
			free(worldScene->polys);
		if (worldScene->materials)
			free(worldScene->materials);
		
		
		time(&stop);
		printf("Finished render in %.0f seconds.\n", difftime(stop, start));

	}
	return 0;
}

