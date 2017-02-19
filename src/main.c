//
//  main.c
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#include "main.h"

typedef struct {
	pthread_t thread_id;
	int thread_num;
	int completedSamples;
	bool threadComplete;
}threadInfo;

//Global variables
world *worldScene = NULL;
double *renderBuffer;
int sectionSize = 0;
int renderThreadCount = 0;
int activeThreads = 0;
bool shouldSave = true;

//SDL globals
SDL_Window *window = NULL;
SDL_Renderer *windowRenderer = NULL;
SDL_Texture *texture = NULL;
bool isRendering = false;
pthread_mutex_t uimutex = PTHREAD_MUTEX_INITIALIZER;

//Thread globals
threadInfo *tinfo;
threadInfo *uitinfo;
pthread_attr_t attributes;
pthread_attr_t uiattributes;

//Function prototypes
void *renderThread(void *arg);
void *drawThread(void *arg);
//void updateProgress(int y, int max, int min);
void updateProgress(int totalSamples, int completedSamples, int threadNum);
void printDuration(double time);
int getFileSize(char *fileName);
color getPixel(world *worldScene, int x, int y);
color rayTrace(lightRay *incidentRay, world *worldScene);
int getSysCores();
vector getRandomVecOnRadius(vector center, float radius);
void getKeyboardInput();

//Signal handling
void (*signal(int signo, void (*func )(int)))(int);
typedef void sigfunc(int);
sigfunc *signal(int, sigfunc*);

void sigHandler(int sig) {
	if (sig == SIGINT) {
		printf("Received CTRL-C, aborting...\n");
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	
	renderThreadCount = getSysCores();
	
	time_t start, stop;
	
	//Seed RNGs
	srand((int)time(NULL));
	srand48(time(NULL));
	
	//Prepare the scene
	world sceneObject;
	sceneObject.materials = NULL;
	sceneObject.spheres = NULL;
	sceneObject.lights = NULL;
	worldScene = &sceneObject; //Assign to global variable
	
	int frame = 0;
	
	//Animation
	do {
		//This is a timer to elapse how long a render takes per frame
		time(&start);
		
		char *fileName = NULL;
		//Build the scene
		if (argc == 2) {
			fileName = argv[1];
		} else {
			logHandler(sceneParseErrorNoPath);
		}
		
		//Build the scene
		switch (testBuild(worldScene, "test")) {
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
		
		float windowScale = worldScene->camera->windowScale;
		
		//Initialize SDL if need be
		if (worldScene->camera->showGUI) {
			if (SDL_Init(SDL_INIT_VIDEO) < 0) {
				fprintf(stdout, "SDL couldn't initialize, error %s\n", SDL_GetError());
				return -1;
			}
			//Init window
			window = SDL_CreateWindow("C-ray © VKoskiv 2015-2017", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, worldScene->camera->width * windowScale, worldScene->camera->height * windowScale, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
			if (window == NULL) {
				fprintf(stdout, "Window couldn't be created, error %s\n", SDL_GetError());
				return -1;
			}
			
			windowRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
			if (windowRenderer == NULL) {
				fprintf(stdout, "Renderer couldn't be created, error %s\n", SDL_GetError());
				return -1;
			}
			SDL_RenderSetScale(windowRenderer, windowScale, windowScale);
			
			texture = SDL_CreateTexture(windowRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, worldScene->camera->width, worldScene->camera->height);
			if (texture == NULL) {
				fprintf(stdout, "Texture couldn't be created, error %s\n", SDL_GetError());
				return -1;
			}
			
		}
		
		//Delay so macOS can draw window border (Yeah...)
		for(int i = 0; i < 50; i++){
			SDL_PumpEvents();
			SDL_Delay(1);
		}
		
		if (worldScene->camera->forceSingleCore) renderThreadCount = 1;
		
		//Create threads
		int t;
		
		//Alloc memory for pthread_create() args
		tinfo = calloc(renderThreadCount, sizeof(threadInfo));
		if (tinfo == NULL) {
			logHandler(threadMallocFailed);
			return -1;
		}
		
		//Alloc memory for uiThread args
		uitinfo = calloc(1, sizeof(threadInfo));
		if (uitinfo == NULL) {
			logHandler(threadMallocFailed);
			return -1;
		}
		
		//Verify sample count
		if (worldScene->camera->sampleCount < 1) logHandler(renderErrorInvalidSampleCount);
		if (!worldScene->camera->areaLights) worldScene->camera->sampleCount = 1;
		
		worldScene->camera->currentFrame = frame;
		frame++;
		
		printf("\nStarting C-ray renderer for frame %i\n\n", worldScene->camera->currentFrame);
		printf("Rendering at %i x %i\n", worldScene->camera->width,worldScene->camera->height);
		printf("Rendering with %i samples\n", worldScene->camera->sampleCount);
		printf("Rendering with %d thread",renderThreadCount);
		if (renderThreadCount > 1) {
			printf("s");
		}
		if (worldScene->camera->forceSingleCore) printf(" (Forced single thread)\n");
		else printf("\n");
		
		printf("Using %i light bounces\n", worldScene->camera->bounces);
		printf("Raytracing...\n");
		
		//Allocate memory and create array of pixels for image data
		worldScene->camera->imgData = (unsigned char*)calloc(4 * worldScene->camera->width * worldScene->camera->height, sizeof(unsigned char));
		
		//Allocate memory for render buffer
		//Render buffer is used to store accurate color values for the renderers' internal use
		renderBuffer = (double*)calloc(4 * worldScene->camera->width * worldScene->camera->height, sizeof(double));
		
		if (!worldScene->camera->imgData) logHandler(imageMallocFailed);
		
		//Calculate section sizes for every thread, multiple threads can't render the same portion of an image
		sectionSize = worldScene->camera->height / renderThreadCount;
		if ((sectionSize % 2) != 0) logHandler(invalidThreadCount);
		isRendering = true;
		pthread_attr_init(&attributes);
		pthread_attr_init(&uiattributes);
		pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
		pthread_attr_setdetachstate(&uiattributes, PTHREAD_CREATE_JOINABLE);
		
		//Main loop (input)
		bool threadsHaveStarted = false;
		while (isRendering) {
			getKeyboardInput();
			
			if (!threadsHaveStarted) {
				threadsHaveStarted = true;
				//Create render threads
				for (t = 0; t < renderThreadCount; t++) {
					tinfo[t].thread_num = t;
					tinfo[t].threadComplete = false;
					activeThreads++;
					if (pthread_create(&tinfo[t].thread_id, &attributes, renderThread, &tinfo[t])) {
						logHandler(threadCreateFailed);
						exit(-1);
					}
				}
				
				uitinfo->threadComplete = false;
				//Create UI render thread
				if (pthread_create(&uitinfo->thread_id, &uiattributes, drawThread, uitinfo)) {
					logHandler(threadCreateFailed);
					exit(-1);
				}
				
				if (pthread_attr_destroy(&attributes)) {
					logHandler(threadRemoveFailed);
				}
			}
			
			//Wait for render threads to finish (Render finished)
			for (t = 0; t < renderThreadCount; t++) {
				if (tinfo[t].threadComplete && tinfo[t].thread_num != -1) {
					activeThreads--;
					tinfo[t].thread_num = -1;
				}
				if (activeThreads == 0) {
					isRendering = false;
				}
			}
			
			//Wait for UI render thread to finish
			if (uitinfo->threadComplete) {
				isRendering = false;
			}
		}
		
		time(&stop);
		printDuration(difftime(stop, start));
		
		//Write to file
		if (shouldSave)
			writeImage(worldScene);
		else
			printf("Image won't be saved!\n");
		
		worldScene->camera->currentFrame++;
		
		//Free memory
		if (worldScene->camera->imgData)
			free(worldScene->camera->imgData);
		if (worldScene->lights)
			free(worldScene->lights);
		if (worldScene->spheres)
			free(worldScene->spheres);
		if (worldScene->materials)
			free(worldScene->materials);
	} while (worldScene->camera->currentFrame < worldScene->camera->frameCount);
	
	printf("Render finished, exiting.\n");
	
	return 0;
}

#pragma mark UI

void *drawThread(void *arg) {
	SDL_SetRenderDrawColor(windowRenderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(windowRenderer);
	while (isRendering) {
		//Check for CTRL-C
		if (signal(SIGINT, sigHandler) == SIG_ERR)
			fprintf(stderr, "Couldn't catch SIGINT\n");
		//Render frame
		pthread_mutex_lock(&uimutex);
		SDL_UpdateTexture(texture, NULL, worldScene->camera->imgData, worldScene->camera->width * 3);
		SDL_RenderCopy(windowRenderer, texture, NULL, NULL);
		SDL_RenderPresent(windowRenderer);
		pthread_mutex_unlock(&uimutex);
		//Print render status
		for (int i = 0; i < renderThreadCount; i++) {
			updateProgress(worldScene->camera->sampleCount, tinfo[i].completedSamples, tinfo[i].thread_num);
		}
	}
	uitinfo->threadComplete = true;
	pthread_exit((void*) arg);
}

void updateProgress(int totalSamples, int completedSamples, int threadNum) {
	printf("Thread %i rendering sample %i/%i\r", threadNum, completedSamples, totalSamples);
	fflush(stdout);
}

void printDuration(double time) {
	if (time <= 60) {
		printf("Finished render in %.0f seconds.\n", time);
	} else if (time <= 3600) {
		printf("Finished render in %.0f minute", time/60);
		if (time/60 > 1) printf("s.\n"); else printf(".\n");
	} else {
		printf("Finished render in %.0f hours (%.0f min).\n", (time/60)/60, time/60);
	}
}

void getKeyboardInput() {
	SDL_PumpEvents();
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_S]) {
		printf("Aborting render, saving...\n");
		isRendering = false;
	}
	if (keys[SDL_SCANCODE_X]) {
		printf("Aborting render without saving...\n");
		shouldSave = false;
		isRendering = false;
	}
}

#pragma mark Renderer

void *renderThread(void *arg) {
	lightRay incidentRay;
	int x,y;
	
	threadInfo *tinfo = (threadInfo*)arg;
	//Figure out which part to render based on current thread ID
	int limits[] = {(tinfo->thread_num * sectionSize), (tinfo->thread_num * sectionSize) + sectionSize};
	int completedSamples = 1;
	
	while (completedSamples < worldScene->camera->sampleCount+1 && isRendering) {
		tinfo->completedSamples = completedSamples;
		for (y = limits[1]; y > limits[0]; y--) {
			for (x = 0; x < worldScene->camera->width; x++) {
				color output = getPixel(worldScene, x, y);
				color sample = {0.0f,0.0f,0.0f,0.0f};
				if (worldScene->camera->viewPerspective.projectionType == ortho) {
					incidentRay.start.x = x/2;
					incidentRay.start.y = y/2;
					incidentRay.start.z = worldScene->camera->pos.z;
					
					incidentRay.direction.x = 0;
					incidentRay.direction.y = 0;
					incidentRay.direction.z = 1;
					sample = rayTrace(&incidentRay, worldScene);
				} else if (worldScene->camera->viewPerspective.projectionType == conic) {
					double focalLength = 0.0f;
					if ((worldScene->camera->viewPerspective.projectionType == conic)
						&& worldScene->camera->viewPerspective.FOV > 0.0f
						&& worldScene->camera->viewPerspective.FOV < 189.0f) {
						focalLength = 0.5f * worldScene->camera->width / tanf((double)(PIOVER180) * 0.5f * worldScene->camera->viewPerspective.FOV);
					}
					
					vector direction = {(x - 0.5f * worldScene->camera->width) / focalLength,
						(y - 0.5f * worldScene->camera->height) / focalLength, 1.0f};
					
					double normal = scalarProduct(&direction, &direction);
					if (normal == 0.0f)
						break;
					direction = vectorScale(invsqrtf(normal), &direction);
					vector startPos = worldScene->camera->pos;
					
					incidentRay.start = startPos;
					incidentRay.direction = direction;
					sample = rayTrace(&incidentRay, worldScene);
					
					output.red = output.red * (completedSamples - 1);
					output.green = output.green * (completedSamples - 1);
					output.blue = output.blue * (completedSamples - 1);
					
					output = addColors(&output, &sample);
					
					output.red = output.red / completedSamples;
					output.green = output.green / completedSamples;
					output.blue = output.blue / completedSamples;
					
					//Store render buffer
					renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 0] = output.red;
					renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 1] = output.green;
					renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 2] = output.blue;
				}
				
				worldScene->camera->imgData[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 0] = (unsigned char)min(  output.red*255.0f, 255.0f);
				worldScene->camera->imgData[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 1] = (unsigned char)min(output.green*255.0f, 255.0f);
				worldScene->camera->imgData[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 2] = (unsigned char)min( output.blue*255.0f, 255.0f);
			}
		}
		completedSamples++;
	}
	printf("Thread %i done\n", tinfo->thread_num);
	tinfo->threadComplete = true;
	pthread_exit((void*) arg);
}

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

color getPixel(world *worldScene, int x, int y) {
	color output = {0.0f, 0.0f, 0.0f, 0.0f};
	output.red = renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 0];
	output.green = renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 1];
	output.blue = renderBuffer[(x + (worldScene->camera->height - y)*worldScene->camera->width)*3 + 2];
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

#pragma mark Helper funcs

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
