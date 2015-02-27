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

//Image dimensions. Eventually get this from the input file
#define kImgWidth 1920
#define kImgHeight 1080
#define kFrameCount 1
#define bounces 2
#define contrast 1.0

//Global variables
unsigned char *imgData = NULL;
unsigned long bounceCount = 0;
randomGenerator = false;

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
		
		world background;
		
		background.ambientColor.red =   0.41;
		background.ambientColor.green = 0.41;
		background.ambientColor.blue =  0.41;
		
		//define materials to an array
		material materials[5];
		
		materials[0].diffuse.red = 1;
		materials[0].diffuse.green = 0;
		materials[0].diffuse.blue = 0;
		materials[0].reflectivity = 0.2;
		
		materials[1].diffuse.red = 0;
		materials[1].diffuse.green = 1;
		materials[1].diffuse.blue = 0;
		materials[1].reflectivity = 0.5;
		
		materials[2].diffuse.red = 0;
		materials[2].diffuse.green = 0;
		materials[2].diffuse.blue = 1;
		materials[2].reflectivity = 1;
		
		materials[3].diffuse.red = 0.3;
		materials[3].diffuse.green = 0.3;
		materials[3].diffuse.blue = 0.3;
		materials[3].reflectivity = 1;
		
		materials[4].diffuse.red = 0;
		materials[4].diffuse.green = 0.517647;
		materials[4].diffuse.blue = 1;
		materials[4].reflectivity = 1;
		
		//Define polygons to an array
		
		//Backup
		/*polys[0].v1.x = 200;
		polys[0].v1.y = 50;
		polys[0].v1.z = 0;
		polys[0].v2.x = kImgWidth-200;
		polys[0].v2.y = 50;
		polys[0].v2.z = 0;
		polys[0].v3.x = kImgWidth/2;
		polys[0].v3.y = kImgHeight/2;
		polys[0].v3.z = 2000;
		polys[0].material = 0;*/
		
		polygonObject polys[4];
		
		//Bottom plane
		polys[0].v1.x = 200;
		polys[0].v1.y = 50;
		polys[0].v1.z = 0;
		
		polys[0].v2.x = kImgWidth-200;
		polys[0].v2.y = 50;
		polys[0].v2.z = 0;
		
		polys[0].v3.x = 400;
		polys[0].v3.y = kImgHeight/2;
		polys[0].v3.z = 2000;
		polys[0].material = 3;
		
		polys[1].v1.x = kImgWidth-200;
		polys[1].v1.y = 50;
		polys[1].v1.z = 0;
		
		polys[1].v2.x = kImgWidth-400;
		polys[1].v2.y = kImgHeight/2;
		polys[1].v2.z = 2000;
		
		polys[1].v3.x = 400;
		polys[1].v3.y = kImgHeight/2;
		polys[1].v3.z = 2000;
		polys[1].material = 3;
		
		//Background plane
		//First poly
		//bottom left
		polys[2].v1.x = 400;
		polys[2].v1.y = kImgHeight/2;
		polys[2].v1.z = 2020;
		//Bottom right
		polys[2].v2.x = kImgWidth-400;
		polys[2].v2.y = kImgHeight/2;
		polys[2].v2.z = 2020;
		//top left
		polys[2].v3.x = 400;
		polys[2].v3.y = kImgHeight;
		polys[2].v3.z = 1500;
		polys[2].material = 3;
		//Second poly
		//top right
		polys[3].v1.x = kImgWidth-400;
		polys[3].v1.y = kImgHeight;
		polys[3].v1.z = 1500;
		//top left
		polys[3].v2.x = 400;
		polys[3].v2.y = kImgHeight;
		polys[3].v2.z = 1500;
		//bottom right
		polys[3].v3.x = kImgWidth-400;
		polys[3].v3.y = kImgHeight/2;
		polys[3].v3.z = 2020;
		polys[3].material = 3;
		
		//define spheres to an array
		//Red sphere
		sphereObject spheres[4];
		spheres[0].pos.x = 400;
		spheres[0].pos.y = 260;
		spheres[0].pos.z = 0;
		spheres[0].radius = 200;
		spheres[0].material = 0;
		
		//green sphere
		spheres[1].pos.x = 650;
		spheres[1].pos.y = 630;
		spheres[1].pos.z = 1750;
		spheres[1].radius = 150;
		spheres[1].material = 1;
		
		//blue sphere
		spheres[2].pos.x = 1300;
		spheres[2].pos.y = 520;
		spheres[2].pos.z = 1000;
		spheres[2].radius = 220;
		spheres[2].material = 2;
		
		//grey sphere
		spheres[3].pos.x = 970;
		spheres[3].pos.y = 250;
		spheres[3].pos.z = 400;
		spheres[3].radius = 100;
		spheres[3].material = 3;
		
		//Define lights to an array
		lightSource lights[1];
		
		lights[0].pos.x = kImgWidth/2;
		lights[0].pos.y = 1080-100;
		lights[0].pos.z = 0;
		lights[0].intensity.red = 0.9;
		lights[0].intensity.green = 0.9;
		lights[0].intensity.blue = 0.9;
		
		/*lights[1].pos.x = 1280;
		lights[1].pos.y = 3000;
		lights[1].pos.z = -1000;
		lights[1].intensity.red = 0.4;
		lights[1].intensity.green = 0.4;
		lights[1].intensity.blue = 0.4;
		
		lights[2].pos.x = 1600;
		lights[2].pos.y = 1000;
		lights[2].pos.z = -1000;
		lights[2].intensity.red = 0.4;
		lights[2].intensity.green = 0.4;
		lights[2].intensity.blue = 0.4;*/
		
		if (randomGenerator) {
			//Define random scene
			int amount = rand()%50+50;
			printf("Spheres: %i\n",amount);
			int i;
			material materials[amount];
			sphereObject spheres[amount];
		 
		 for (i = 0; i < amount; i++) {
			 materials[i].diffuse.red = randRange(0,1);
			 materials[i].diffuse.green = randRange(0,1);
			 materials[i].diffuse.blue = randRange(0,1);
			 materials[i].reflectivity = randRange(0,1);
			 
			 spheres[i].pos.x = rand()%kImgWidth;
			 spheres[i].pos.y = rand()%kImgWidth;
			 spheres[i].pos.z = rand()&4000-2000;
			 spheres[i].radius = rand()%100+50;
			 spheres[i].material = i;
		 }
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
		//unsigned char imgData[3*kImgWidth*kImgHeight];
		
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
					int sphereAmount = (unsigned int)sizeof(spheres)/sizeof(sphereObject);
					int polygonAmount = sizeof(polys)/sizeof(polygonObject);
					int lightSourceAmount = sizeof(lights)/sizeof(lightSource);
					
					material currentMaterial;
					vector polyNormal, hitpoint, surfaceNormal;
					
					unsigned int i;
					for (i = 0; i < sphereAmount; i++) {
						if (rayIntersectsWithSphere(&incidentRay, &spheres[i], &t)) {
							currentSphere = i;
						}
					}
					
					for (i = 0; i < polygonAmount; i++) {
						if (rayIntersectsWithPolygon(&incidentRay, &polys[i], &t, &polyNormal)) {
							currentPolygon = i;
							currentSphere = -1;
						}
					}
					
					//Ray-object intersection detection
					if (currentSphere != -1) {
						bounceCount++;
						vector scaled = vectorScale(t, &incidentRay.direction);
						hitpoint = addVectors(&incidentRay.start, &scaled);
						surfaceNormal = subtractVectors(&hitpoint, &spheres[currentSphere].pos);
						temp = scalarProduct(&surfaceNormal,&surfaceNormal);
						if (temp == 0.0f) break;
						temp = FastInvSqrt(temp);
						surfaceNormal = vectorScale(temp, &surfaceNormal);
						currentMaterial = materials[spheres[currentSphere].material];
					} else if (currentPolygon != -1) {
						bounceCount++;
						vector scaled = vectorScale(t, &incidentRay.direction);
						hitpoint = addVectors(&incidentRay.start, &scaled);
						surfaceNormal = polyNormal;
						temp = scalarProduct(&surfaceNormal,&surfaceNormal);
						if (temp == 0.0f) break;
						temp = FastInvSqrt(temp);
						surfaceNormal = vectorScale(temp, &surfaceNormal);
						currentMaterial = materials[polys[currentPolygon].material];
					} else {
						//Ray didn't hit any object, set color to ambient
						color back;
						back.red = background.ambientColor.red;
						back.green = background.ambientColor.green;
						back.blue = background.ambientColor.blue;
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
							lightSource currentLight = lights[j];
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
								if (rayIntersectsWithSphere(&bouncedRay, &spheres[k], &t)) {
									inShadow = true;
									break;
								}
							}
							
							for (k = 0; k < polygonAmount; ++k) {
								if (rayIntersectsWithPolygon(&bouncedRay, &polys[k], &t, &polyNormal)) {
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
		
		if (imgData) {
			free(imgData);
			printf("Released image data array\n");
		}
		time(&stop);
		printf("Finished render in %.0f seconds.\n", difftime(stop, start));

	}
	return 0;
}

