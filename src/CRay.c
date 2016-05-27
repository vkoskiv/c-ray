//
//  CRay.c
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//
/*
 TODO:
 (Add antialiasing)
 (Add total render time for animations)
 Add programmatic textures (checker pattern)
 Add refraction (Glass)
 Soft shadows
 Texture mapping
 Tiled rendering
 Per-thread progress log
 */

#include <pthread.h>
#include "CRay.h"

//These are for multi-platform physical core detection
#ifdef MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#elif _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

//Global variables
world *worldScene = NULL;
//Raw image data array
unsigned char *imgData = NULL;
unsigned long bounceCount = 0;
int sectionSize = 0;

//Prototypes
void *renderThread(void *arg);
color rayTrace(lightRay *incidentRay, world *worldScene);
color rayTrace2(lightRay *incidentRay, world *worldScene);
int getSysCores();

//Thread
typedef struct {
	pthread_t thread_id;
	int thread_num;
}threadInfo;

int main(int argc, char *argv[]) {
	
    int renderThreads = getSysCores();
	
	//Free image array for safety
    if (imgData) free(imgData);
	
	time_t start, stop;
	
	//Seed RNGs
	srand((int)time(NULL));
	srand48(time(NULL));
	
	//Animation
	for (int currentFrame = 0; currentFrame < kFrameCount; currentFrame++) {
		//This is a timer to elapse how long a render takes per frame
		time(&start);
		
		//Prepare the scene
		world sceneObject;
		sceneObject.materials = NULL;
		sceneObject.spheres = NULL;
		sceneObject.polys = NULL;
		sceneObject.lights = NULL;
		worldScene = &sceneObject; //Assign to global variable
		
		//Create threads
		threadInfo *tinfo;
		pthread_attr_t attributes;
		int t;
		
		//Alloc memory for pthread_create() args
		tinfo = calloc(renderThreads, sizeof(threadInfo));
		if (tinfo == NULL) {
			logHandler(threadMallocFailed);
			return -1;
		}
		
		//Build the scene
        char *fileName = "../output/scene.txt";
        int returnValue = buildScene(worldScene, fileName);
        switch (returnValue) {
            case -1:
                logHandler(sceneBuildFailed);
                break;
            case -2:
                logHandler(sceneParseErrorMalloc);
                break;
            case 4:
                logHandler(sceneDebugEnabled);
                return 0;
                break;
            default:
                break;
        }
        
        printf("Starting C-ray renderer\n\n");
		printf("Rendering at %i x %i\n",worldScene->camera.width,worldScene->camera.height);
		printf("Rendering with %d thread",renderThreads);
		if (renderThreads < 2) {
			printf("\n");
		} else {
			printf("s\n");
		}
		
		printf("Using %i light bounces\n",worldScene->camera.bounces);
		printf("Raytracing...\n");
		//Allocate memory and create array of pixels for image data
		imgData = (unsigned char*)malloc(4 * worldScene->camera.width * worldScene->camera.height);
		memset(imgData, 0, 4 * worldScene->camera.width * worldScene->camera.height);
		
        if (!imgData) logHandler(imageMallocFailed);
        
		//Calculate section sizes for every thread, multiple threads can't render the same portion of an image
		sectionSize = worldScene->camera.height / renderThreads;
		if ((sectionSize % 2) != 0) logHandler(invalidThreadCount);
		pthread_attr_init(&attributes);
		pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
		
		//Create render threads
		for (t = 0; t < renderThreads; t++) {
			tinfo[t].thread_num = t;
			if (pthread_create(&tinfo[t].thread_id, &attributes, renderThread, &tinfo[t])) {
				logHandler(threadCreateFailed);
				exit(-1);
			}
		}
		
		if (pthread_attr_destroy(&attributes)) {
			logHandler(threadRemoveFailed);
		}
		
		//Wait for render threads to finish (Render finished)
		for (t = 0; t < renderThreads; t++) {
			if (pthread_join(tinfo[t].thread_id, NULL)) {
				logHandler(threadFrozen);
			}
		}
		
		time(&stop);
		printf("Finished render in %.0f seconds.\n", difftime(stop, start));
		
		printf("%lu light bounces total\n",bounceCount);
		printf("Saving result\n");
		//Save image data to a file
		//String manipulation is lovely in C
		//FIXME: This crashes if frame count is over 9999
		int bufSize;
		if (currentFrame < 100) {
			bufSize = 26;
		} else if (currentFrame < 1000) {
			bufSize = 27;
		} else {
			bufSize = 28;
		}
		char buf[bufSize];
        
        if (worldScene->camera.outputFileType == ppm) {
            sprintf(buf, "../output/rendered_%d.ppm", currentFrame);
            printf("%s\n", buf);
            saveImageFromArray(buf, imgData, worldScene->camera.width, worldScene->camera.height);
        } else if (worldScene->camera.outputFileType == bmp){
            sprintf(buf, "../output/rendered_%d.bmp", currentFrame);
            printf("%s\n", buf);
            saveBmpFromArray(buf, imgData, worldScene->camera.width, worldScene->camera.height);
        } else {
            sprintf(buf, "../output/rendered_%d.png", currentFrame);
            printf("%s\n", buf);
            encodePNG(buf, imgData, worldScene->camera.width, worldScene->camera.height);
        }
        
		long bytes = 3 * worldScene->camera.width * worldScene->camera.height;
		long mBytes = (bytes / 1000) / 1000;
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
	}
    if (kFrameCount > 0) {
        printf("Animation render finished\n");
    }
	return 0;
}

color rayTrace2(lightRay *viewRay, world *worldScene) {
    color output = *worldScene->ambientColor;
    int bounces = 0;
    double contrast = worldScene->camera.contrast;
    
    while ((contrast > 0.0f) && (bounces <= worldScene->camera.bounces)) {
        double maxDistance = 20000.0f;
        double temp;
        
        sphereObject currentSphere;
        polygonObject currentPoly;
        //lightSphere currentLight;
        material currentMaterial;
        vector normal, hitPoint, surfaceNormal;
        
        currentSphere.active = false;
        currentPoly.active = false;
        
        //Loop through objects in scene to see which one will be raytraced
        unsigned int i;
        
        for (i = 0; i < worldScene->polygonAmount; ++i) {
            if (rayIntersectsWithPolygon(viewRay, &worldScene->polys[i], &maxDistance, &normal)) {
                currentPoly = worldScene->polys[i];
                currentPoly.active = true;
            }
        }
        
        for (i = 0; i < worldScene->sphereAmount; ++i) {
            if (rayIntersectsWithSphere(viewRay, &worldScene->spheres[i], &maxDistance)) {
                currentSphere = worldScene->spheres[i];
                currentSphere.active = true;
            }
        }
        
        //Calculate ray-object intersection point
        if (currentPoly.active) {
            bounceCount++;
            vector scaled = vectorScale(maxDistance, &viewRay->direction);
            hitPoint = addVectors(&viewRay->start, &scaled);
            surfaceNormal = normal;
            temp = scalarProduct(&surfaceNormal,&surfaceNormal);
            if (temp == 0.0f) break;
            temp = invsqrtf(temp);
            surfaceNormal = vectorScale(temp, &surfaceNormal);
            currentMaterial = worldScene->materials[currentPoly.material];
        } else if (currentSphere.active) {
            bounceCount++;
            vector scaled = vectorScale(maxDistance, &viewRay->direction);
            hitPoint = addVectors(&viewRay->start, &scaled);
            surfaceNormal = subtractVectors(&hitPoint, &currentSphere.pos);
            temp = scalarProduct(&surfaceNormal,&surfaceNormal);
            if (temp == 0.0f) break;
            temp = invsqrtf(temp);
            surfaceNormal = vectorScale(temp, &surfaceNormal);
            currentMaterial = worldScene->materials[currentSphere.material];
        } else {
            //Ray didn't hit anything, return ambient color.
            color temp = colorCoef(contrast, &output);
            return addColors(&output, &temp);
        }
        
    }
    return output;
}

color rayTrace(lightRay *incidentRay, world *worldScene) {
	//Raytrace a given light ray with a given scene, then return the color value for that ray
	color output = {0.0f,0.0f,0.0f};
	int bounces = 0;
	double contrast = worldScene->camera.contrast;

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
		for (i = 0; i < sphereAmount; ++i) {
			if (rayIntersectsWithSphere(incidentRay, &worldScene->spheres[i], &t)) {
				currentSphere = i;
			}
		}
		
		for (i = 0; i < polygonAmount; ++i) {
			if (rayIntersectsWithPolygon(incidentRay, &worldScene->polys[i], &t, &polyNormal)) {
				currentPolygon = i;
				currentSphere = -1;
			}
		}
		
		//Ray-object intersection detection
		if (currentSphere != -1) {
			bounceCount++;
			vector scaled = vectorScale(t, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			surfaceNormal = subtractVectors(&hitpoint, &worldScene->spheres[currentSphere].pos);
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			surfaceNormal = vectorScale(temp, &surfaceNormal);
			currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].material];
		} else if (currentPolygon != -1) {
			bounceCount++;
			vector scaled = vectorScale(t, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			surfaceNormal = polyNormal;
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			surfaceNormal = vectorScale(temp, &surfaceNormal);
			currentMaterial = worldScene->materials[worldScene->polys[currentPolygon].material];
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
        cameraRay.direction = subtractVectors(&worldScene->camera.pos, &hitpoint);
        double cameraProjection = scalarProduct(&cameraRay.direction, &hitpoint);
        //if (cameraProjection <= 0.0f) continue;
        double cameraDistance = scalarProduct(&cameraRay.direction, &cameraRay.direction);
        double camTemp = cameraDistance;
        //if (camTemp == 0.0f) continue;
        camTemp = invsqrtf(camTemp);
        cameraRay.direction = vectorScale(camTemp, &cameraRay.direction);
        cameraProjection = camTemp * cameraProjection;
        //Find the value of the light at this point (Scary!)
        unsigned int j;
        for (j = 0; j < lightSourceAmount; ++j) {
            lightSphere currentLight = worldScene->lights[j];
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
                //TODO: Calculate specular reflection
                float specularFactor = 1.0; //scalarProduct(&cameraRay.direction, &surfaceNormal) * contrast;
                
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
		
	} while ((contrast > 0.0f) && (bounces <= worldScene->camera.bounces));
	
	return output;
}

void *renderThread(void *arg) {
	lightRay incidentRay;
	int x,y;
	
	threadInfo *tinfo = (threadInfo*)arg;
	//Figure out which part to render based on current thread ID
	int limits[] = {(tinfo->thread_num * sectionSize), (tinfo->thread_num * sectionSize) + sectionSize};
	
	for (y = limits[0]; y < limits[1]; y++) {
		for (x = 0; x < worldScene->camera.width; x++) {
			color output = {0.0f,0.0f,0.0f,0.0f};
            double fragX, fragY;
			if (worldScene->camera.viewPerspective.projectionType == ortho) {
				//Fix these
				incidentRay.start.x = x;
				incidentRay.start.y = y ;
				incidentRay.start.z = -2000;
				
				incidentRay.direction.x = 0;
				incidentRay.direction.y = 0;
				incidentRay.direction.z = 1;
				output = rayTrace(&incidentRay, worldScene);
			} else if (worldScene->camera.antialiased == false && worldScene->camera.viewPerspective.projectionType == conic) {
				double focalLength = 0.0f;
				if ((worldScene->camera.viewPerspective.projectionType == conic)
					&& worldScene->camera.viewPerspective.FOV > 0.0f
					&& worldScene->camera.viewPerspective.FOV < 189.0f) {
					focalLength = 0.5f * worldScene->camera.width / tanf((double)(PIOVER180) * 0.5f * worldScene->camera.viewPerspective.FOV);
				}
				
				vector direction = {((x - 0.5f * worldScene->camera.width)/focalLength) +
					worldScene->camera.lookAt.x, ((y - 0.5f * worldScene->camera.height)/focalLength) +
					worldScene->camera.lookAt.y, 1.0f};
				
				double normal = scalarProduct(&direction, &direction);
				if (normal == 0.0f)
					break;
				direction = vectorScale(invsqrtf(normal), &direction);
				vector startPos = {worldScene->camera.pos.x, worldScene->camera.pos.y, worldScene->camera.pos.z};
                incidentRay.start = startPos;
				
				incidentRay.direction = direction;
				output = rayTrace(&incidentRay, worldScene);
            } else if (worldScene->camera.antialiased == true && worldScene->camera.viewPerspective.projectionType == conic) {
                for (fragX = x; fragX < x + 1.0f; fragX += 0.5) {
                    for (fragY = y; fragY < y + 1.0f; fragY += 0.5f) {
                        double focalLength = 0.0f;
                        if ((worldScene->camera.viewPerspective.projectionType == conic)
                            && worldScene->camera.viewPerspective.FOV > 0.0f
                            && worldScene->camera.viewPerspective.FOV < 189.0f) {
                            focalLength = 0.5f * worldScene->camera.width / tanf((double)(PIOVER180) * 0.5f * worldScene->camera.viewPerspective.FOV);
                            
                            vector direction = {((fragX - 0.5f * worldScene->camera.width) / focalLength) + worldScene->camera.lookAt.x,
                                                ((fragY - 0.5f * worldScene->camera.height) / focalLength) + worldScene->camera.lookAt.y,
                                                1.0f
                                                };
                            
                            double normal = scalarProduct(&direction, &direction);
                            if (normal == 0.0f)
                                break;
                            direction = vectorScale(invsqrtf(normal), &direction);
                            vector startPos = {worldScene->camera.pos.x, worldScene->camera.pos.y, worldScene->camera.pos.z};
                            incidentRay.start = startPos;
                            
							incidentRay.direction = direction;
                            output = rayTrace(&incidentRay, worldScene);
                        }
                    }
                }
            }
            //imgData[(x + y*worldScene->camera.width)*3 + 3] = (unsigned char)min(output.alpha*255.0f, 255.0f);
			imgData[(x + y*worldScene->camera.width)*3 + 2] = (unsigned char)min(  output.red*255.0f, 255.0f);
			imgData[(x + y*worldScene->camera.width)*3 + 1] = (unsigned char)min(output.green*255.0f, 255.0f);
			imgData[(x + y*worldScene->camera.width)*3 + 0] = (unsigned char)min( output.blue*255.0f, 255.0f);
		}
	}
	pthread_exit((void*) arg);
}

int getSysCores() {
#ifdef MACOS
	int nm[2];
	size_t len = 4;
	uint32_t count;
	
	nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
	sysctl(nm, 2, &count, &len, NULL, 0);
	
	if (count < 1) {
		nm[1] = HW_NCPU;
		sysctl(nm, 2, &count, &len, NULL, 0);
		if (count < 1) {
			count = 1;
		}
	}
	return count;
#elif WIN32
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysinfo.dwNumberOfProcessors;
#else
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

float randRange(float a, float b) {
	return ((b-a)*((float)rand()/RAND_MAX))+a;
}

double rads(double angle) {
    return PIOVER180 * angle;
}