//
//  scene.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "scene.h"

#define TOKEN_DEBUG_ENABLED false

//FIXME: Spheres and lights don't use vertex indices yet
//FIXME: lightSphere->pointLight, sphereAmount->sphereCount etc

//Prototypes
//Trims spaces and tabs from a char array
char *trim_whitespace(char *inputLine);
//Parses a scene file and allocates memory accordingly
int allocMemory(world *scene, char *inputFileName);

/*int buildScene(world *scene, char *inputFileName) {
    printf("\nStarting C-ray Scene Parser 0.2\n");
    FILE *inputFile = fopen(inputFileName, "r");
    if (!inputFile)
        return -1;
    char *delimEquals = "=", *delimComma = ",", *closeBlock = "}";
    char *error = NULL;
    char *token;
	char *savePointer;
	int materialIndex = 0, sphereIndex = 0, polyIndex = 0, lightIndex = 0;
    if (allocMemory(scene, inputFileName) == -1)
        return -2;
	
	if (scene->objCount > 0) {
		loadOBJ(scene, "filename here somehow -.-");
	}
	
    char line[255];
    while (fgets(line, sizeof(line), inputFile) != NULL) {
        //Discard comments
        if (trim_whitespace(line)[0] == '#') {
            //Ignore
        }
        if (strcmp(trim_whitespace(line), "scene(){\n") == 0) {
            while (trim_whitespace(line)[0] != *closeBlock) {
                error = fgets(trim_whitespace(line), sizeof(line), inputFile);
                if (!error)
                    logHandler(sceneParseErrorSphere);
				if (strncmp(trim_whitespace(line), "ambientColor", 12) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					scene->ambientColor = (color * )calloc(3, sizeof(color));
					token = strtok_r(NULL, delimComma, &savePointer);  //Red
					float rgbValue = strtof(token, NULL);
					scene->ambientColor->red = rgbValue;
					token = strtok_r(NULL, delimComma, &savePointer);  //Green
					rgbValue = strtof(token, NULL);
					scene->ambientColor->green = rgbValue;
					token = strtok_r(NULL, delimComma, &savePointer);  //Blue
					rgbValue = strtof(token, NULL);
					scene->ambientColor->blue = rgbValue;
				}
            }
        }
		
		if (strcmp(trim_whitespace(line), "camera(){\n") == 0) {
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorCamera);
				if (strncmp(trim_whitespace(line), "perspective", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					
					if (strncmp(savePointer, "conic", 5) == 0) {
						scene->camera->viewPerspective.projectionType = conic;
					} else if (strncmp(savePointer, "ortho", 5) == 0){
						scene->camera->viewPerspective.projectionType = ortho;
					}
				}
                
                if (strncmp(trim_whitespace(line), "fileFormat", 10) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorCamera);
                    
                    if (strncmp(savePointer, "bmp", 3) == 0) {
						scene->camera->fileType = bmp;
                    }else if (strncmp(savePointer, "png", 3) == 0) {
                        scene->camera->fileType = png;
                    }
                }
				
                if (strncmp(trim_whitespace(line), "forceSingleCore", 15) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorCamera);
                    if (strncmp(savePointer, "true", 4) == 0) {
						scene->camera->forceSingleCore = true;
                    } else if (strncmp(savePointer, "false", 5) == 0) {
						scene->camera->forceSingleCore = false;
                    }
                }
				
				if (strncmp(trim_whitespace(line), "FOV", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					float FOV = strtof(savePointer, NULL);
					scene->camera->viewPerspective.FOV = FOV;
				}
				
				if (strncmp(trim_whitespace(line), "contrast", 8) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					float contrast = strtof(savePointer, NULL);
					scene->camera->contrast = contrast;
				}
				
				if (strncmp(trim_whitespace(line), "antialiased", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					if (strncmp(savePointer, "true", 4) == 0) {
						scene->camera->antialiased = true;
					} else if (strncmp(savePointer, "false", 5) == 0) {
						scene->camera->antialiased = false;
					}
				}
				
				if (strncmp(trim_whitespace(line), "sampleCount", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int sampleCount = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->sampleCount = sampleCount;
				}
				
				if (strncmp(trim_whitespace(line), "bounces", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int bounces = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->bounces = bounces;
				}
				
				if (strncmp(trim_whitespace(line), "frameCount", 10) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int frameCount = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->frameCount = frameCount;
				}
				
				if (strncmp(trim_whitespace(line), "posX", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->pos.x = posX;
				}
				
				if (strncmp(trim_whitespace(line), "posY", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->pos.y = posY;
				}
				
				if (strncmp(trim_whitespace(line), "posZ", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posZ = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->pos.z = posZ;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtX", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->lookAt.x = lookAtX;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtY", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->lookAt.y = lookAtY;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtZ", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtZ = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->lookAt.z = lookAtZ;
				}
				
				if (strncmp(trim_whitespace(line), "resolutionX", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int resolutionX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->width = resolutionX;
				}
				
				if (strncmp(trim_whitespace(line), "resolutionY", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int resolutionY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera->height = resolutionY;
				}
			}
		}
		
		if (strcmp(trim_whitespace(line), "material(){\n") == 0) {
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorMaterial);
				if (strncmp(trim_whitespace(line), "red", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float red = strtof(savePointer, NULL);
					scene->materials[materialIndex].diffuse.red = red;
				}
				
				if (strncmp(trim_whitespace(line), "green", 5) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float green = strtof(savePointer, NULL);
					scene->materials[materialIndex].diffuse.green = green;
				}
				
				if (strncmp(trim_whitespace(line), "blue", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float blue = strtof(savePointer, NULL);
					scene->materials[materialIndex].diffuse.blue = blue;
				}
				
				if (strncmp(trim_whitespace(line), "reflectivity", 12) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float reflectivity = strtof(savePointer, NULL);
					scene->materials[materialIndex].reflectivity = reflectivity;
				}
			}
			materialIndex++;
		}
		
		if (strcmp(trim_whitespace(line), "light(){\n") == 0) {
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorLight);
				if (strncmp(trim_whitespace(line), "posX", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					int posX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->lights[lightIndex].pos.x = posX;
				}
				
				if (strncmp(trim_whitespace(line), "posY", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					int posY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->lights[lightIndex].pos.y = posY;
				}
				
				if (strncmp(trim_whitespace(line), "posZ", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					int posZ = (int)strtol(savePointer, (char**)NULL, 10);
					scene->lights[lightIndex].pos.z = posZ;
				}
				
				if (strncmp(trim_whitespace(line), "red", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float red = strtof(savePointer, NULL);
					scene->lights[lightIndex].intensity.red = red;
				}
				
				if (strncmp(trim_whitespace(line), "green", 5) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float green = strtof(savePointer, NULL);
					scene->lights[lightIndex].intensity.green = green;
				}
				
				if (strncmp(trim_whitespace(line), "blue", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float blue = strtof(savePointer, NULL);
					scene->lights[lightIndex].intensity.blue = blue;
				}
				
				if (strncmp(trim_whitespace(line), "radius", 6) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float radius = strtof(savePointer, NULL);
					scene->lights[lightIndex].radius = radius;
				}
			}
			lightIndex++;
		}
		
		if (strcmp(trim_whitespace(line), "sphere(){\n") == 0) {
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorSphere);
				if (strncmp(trim_whitespace(line), "posX", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int posX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->spheres[sphereIndex].pos.x = posX;
				}
				
				if (strncmp(trim_whitespace(line), "posY", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int posY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->spheres[sphereIndex].pos.y = posY;
				}
				
				if (strncmp(trim_whitespace(line), "posZ", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int posZ = (int)strtol(savePointer, (char**)NULL, 10);
					scene->spheres[sphereIndex].pos.z = posZ;
				}
				
				if (strncmp(trim_whitespace(line), "radius", 6) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					float radius = strtof(savePointer, NULL);
					scene->spheres[sphereIndex].radius = radius;
				}
				
				if (strncmp(trim_whitespace(line), "material", 8) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int material = (int)strtol(savePointer, (char**)NULL, 10);
					scene->spheres[sphereIndex].material = material;
				}
			}
			sphereIndex++;
		}
		if (strcmp(trim_whitespace(line), "poly(){\n") == 0) {
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorPoly);
                if (strncmp(trim_whitespace(line), "v1X", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v1X = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v1.x = v1X;
                }
                
                if (strncmp(trim_whitespace(line), "v1Y", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v1Y = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v1.y = v1Y;
                }
                
                if (strncmp(trim_whitespace(line), "v1Z", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v1Z = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v1.z = v1Z;
                }
                
                
                if (strncmp(trim_whitespace(line), "v2X", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v2X = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v2.x = v2X;
                }
                
                if (strncmp(trim_whitespace(line), "v2Y", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v2Y = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v2.y = v2Y;
                }
                
                if (strncmp(trim_whitespace(line), "v2Z", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v2Z = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v2.z = v2Z;
                }
                
                
                if (strncmp(trim_whitespace(line), "v3X", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v3X = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v3.x = v3X;
                }
                
                if (strncmp(trim_whitespace(line), "v3Y", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v3Y = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v3.y = v3Y;
                }
                
                if (strncmp(trim_whitespace(line), "v3Z", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v3Z = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].v3.z = v3Z;
                }
                
                if (strncmp(trim_whitespace(line), "material", 8) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int material = (int)strtol(savePointer, (char**)NULL, 10);
                    scene->polys[polyIndex].material = material;
                }
			}
			polyIndex++;
		}
    }
    fclose(inputFile);
    printf("\n");
    
    printf("Finished parsing, found:\n");
    printf("%i lights\n", scene->lightAmount);
    printf("%i spheres\n", scene->sphereAmount);
    printf("%i polygons\n", scene->polygonAmount);
    printf("%i materials\n", scene->materialAmount);
    
    if (TOKEN_DEBUG_ENABLED) {
        return 4; //Debug mode - Won't render anything
    } else {
        return 0;
    }
}*/

int testBuild(world *scene, char *inputFileName) {
	scene->  sphereAmount = 3;
	scene-> polygonAmount = 13;//13;
	scene->materialAmount = 10;
	scene->   lightAmount = 2;
	
	scene->camera = (camera*)calloc(1, sizeof(camera));
	//General scene params
	scene->camera->width = 2560;
	scene->camera->height = 1600;
	scene->camera->fileType = png;
	scene->camera->viewPerspective.projectionType = conic ;
	scene->camera->viewPerspective.FOV = 80.0;
	scene->camera->antialiased = false;
	scene->camera->forceSingleCore = false;
	scene->camera->sampleCount = 10; //Unused
	scene->camera->frameCount = 1;
	scene->camera->bounces = 3;
	scene->camera->contrast = 1.0;
	scene->camera->posIndex = 20;
	scene->camera->lookAtIndex = 21;
	
	scene->ambientColor = (color*)calloc(1, sizeof(color));
	scene->ambientColor->red = 0.4;
	scene->ambientColor->green = 0.6;
	scene->ambientColor->blue = 0.6;
	
	vertexCount = 22;
	vertexArray = (vector*)calloc(vertexCount, sizeof(vector));
	
	//Hard coded vertices for this test
	//Vertices
	//FLOOR
	vertexArray[0] = vectorWithPos(200,300,0);
	vertexArray[1] = vectorWithPos(1720,300,0);
	vertexArray[2] = vectorWithPos(200,300,2000);
	vertexArray[3] = vectorWithPos(1720,300,2000);
	//CEILING
	vertexArray[4] = vectorWithPos(200,840,0);
	vertexArray[5] = vectorWithPos(1720,840,0);
	vertexArray[6] = vectorWithPos(200,840,2000);
	vertexArray[7] = vectorWithPos(1720,840,2000);
	
	//MIRROR PLANE
	vertexArray[8] = vectorWithPos(1420,750,1800);
	vertexArray[9] = vectorWithPos(1420,300,1800);
	vertexArray[10] = vectorWithPos(1620,750,1700);
	vertexArray[11] = vectorWithPos(1620,300,1700);
	//SHADOW TEST
	vertexArray[12] = vectorWithPos(1000,450,1400);
	vertexArray[13] = vectorWithPos(1300,700,1400);
	vertexArray[14] = vectorWithPos(1000,700,1600);
	//LIGHTS
	vertexArray[15] = vectorWithPos(960,400,0);
	vertexArray[16] = vectorWithPos(960,500,0);
	//SPHERES
	vertexArray[17] = vectorWithPos(650,450,1650);
	vertexArray[18] = vectorWithPos(950,350,1000);
	vertexArray[19] = vectorWithPos(1100,350,1000);
	//CAMERA
	vertexArray[20] = vectorWithPos(940,480,0);
	vertexArray[21] = vectorWithPos(0,0,1);
	
	//1
	//3
	//5
	//7
	
	//POLYGONS
	
	scene->polys = (poly*)calloc(scene->polygonAmount, sizeof(poly));
	
	//FLOOR
	scene->polys[0].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[0].vertexIndex[0] = 0;
	scene->polys[0].vertexIndex[1] = 1;
	scene->polys[0].vertexIndex[2] = 2;
	scene->polys[0].materialIndex = 3;
	scene->polys[1].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[1].vertexIndex[0] = 1;
	scene->polys[1].vertexIndex[1] = 2;
	scene->polys[1].vertexIndex[2] = 3;
	scene->polys[1].materialIndex = 3;
	
	//CEILING
	scene->polys[2].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[2].vertexIndex[0] = 4;
	scene->polys[2].vertexIndex[1] = 5;
	scene->polys[2].vertexIndex[2] = 6;
	scene->polys[2].materialIndex = 3;
	scene->polys[3].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[3].vertexIndex[0] = 5;
	scene->polys[3].vertexIndex[1] = 6;
	scene->polys[3].vertexIndex[2] = 7;
	scene->polys[3].materialIndex = 3;
	
	//MIRROR PLANE
	scene->polys[4].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[4].vertexIndex[0] = 8;
	scene->polys[4].vertexIndex[1] = 9;
	scene->polys[4].vertexIndex[2] = 10;
	scene->polys[4].materialIndex = 5;
	scene->polys[5].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[5].vertexIndex[0] = 9;
	scene->polys[5].vertexIndex[1] = 10;
	scene->polys[5].vertexIndex[2] = 11;
	scene->polys[5].materialIndex = 5;
	
	//SHADOW TEST POLY
	scene->polys[6].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[6].vertexIndex[0] = 12;
	scene->polys[6].vertexIndex[1] = 13;
	scene->polys[6].vertexIndex[2] = 14;
	scene->polys[6].materialIndex = 4;
	
	//REAR WALL
	scene->polys[7].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[7].vertexIndex[0] = 2;
	scene->polys[7].vertexIndex[1] = 3;
	scene->polys[7].vertexIndex[2] = 6;
	scene->polys[7].materialIndex = 0;
	scene->polys[8].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[8].vertexIndex[0] = 3;
	scene->polys[8].vertexIndex[1] = 6;
	scene->polys[8].vertexIndex[2] = 7;
	scene->polys[8].materialIndex = 0;
	
	//LEFT WALL
	scene->polys[9].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[9].vertexIndex[0] = 0;
	scene->polys[9].vertexIndex[1] = 2;
	scene->polys[9].vertexIndex[2] = 4;
	scene->polys[9].materialIndex = 1;
	scene->polys[10].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[10].vertexIndex[0] = 2;
	scene->polys[10].vertexIndex[1] = 4;
	scene->polys[10].vertexIndex[2] = 6;
	scene->polys[10].materialIndex = 1;
	
	//RIGHT WALL
	scene->polys[11].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[11].vertexIndex[0] = 1;
	scene->polys[11].vertexIndex[1] = 3;
	scene->polys[11].vertexIndex[2] = 5;
	scene->polys[11].materialIndex = 2;
	scene->polys[12].vertexCount = MAX_VERTEX_COUNT;
	scene->polys[12].vertexIndex[0] = 3;
	scene->polys[12].vertexIndex[1] = 5;
	scene->polys[12].vertexIndex[2] = 7;
	scene->polys[12].materialIndex = 2;
	
	//MATERIALS
	scene->materials = (material*)calloc(scene->materialAmount, sizeof(material));
	scene->materials[0].diffuse = colorWithValues(0.8, 0.1, 0.1, 0);
	scene->materials[0].reflectivity = 0;
	scene->materials[1].diffuse = colorWithValues(0.1, 0.8, 0.1, 0);
	scene->materials[1].reflectivity = 0;
	scene->materials[2].diffuse = colorWithValues(0.1, 0.1, 0.8, 0);
	scene->materials[2].reflectivity = 0;
	scene->materials[3].diffuse = colorWithValues(0.8, 0.8, 0.8, 0);
	scene->materials[3].reflectivity = 0;
	scene->materials[4].diffuse = colorWithValues(0, 0.517647, 1.0, 0);
	scene->materials[4].reflectivity = 1;
	scene->materials[5].diffuse = colorWithValues(0.3, 0.3, 0.3, 0);
	scene->materials[5].reflectivity = 1;
	scene->materials[6].diffuse = colorWithValues(0.3, 0, 0, 0);
	scene->materials[6].reflectivity = 1;
	scene->materials[7].diffuse = colorWithValues(0, 0.3, 0, 0);
	scene->materials[7].reflectivity = 1;
	scene->materials[8].diffuse = colorWithValues(0, 0, 0.3, 0);
	scene->materials[8].reflectivity = 0;
	scene->materials[9].diffuse = colorWithValues(0.9, 0.9, 0.9, 0);
	scene->materials[9].reflectivity = 0;
	
	scene->lights = (lightSphere*)calloc(scene->lightAmount, sizeof(lightSphere));
	scene->lights[0].pos = vertexArray[15];
	scene->lights[0].intensity = colorWithValues(1, 1, 1, 0);
	
	scene->lights[1].pos = vertexArray[16];
	scene->lights[1].intensity = colorWithValues(0.8, 0.8, 0.8, 0);
	
	scene->spheres = (sphere*)calloc(scene->sphereAmount, sizeof(sphere));
	scene->spheres[0].pos = vertexArray[17];
	scene->spheres[0].material = 5;
	scene->spheres[0].radius = 150;
	
	scene->spheres[1].pos = vertexArray[18];
	scene->spheres[1].material = 6;
	scene->spheres[1].radius = 50;
	
	scene->spheres[2].pos = vertexArray[19];
	scene->spheres[2].material = 8;
	scene->spheres[2].radius = 50;
	
	return 0;
}

int allocMemory(world *scene, char *inputFileName) {
    int materialCount = 0, lightCount = 0, polyCount = 0, sphereCount = 0, objCount = 0;
    FILE *inputFile = fopen(inputFileName, "r");
    if (!inputFile)
        return -1;
    char line[255];
    while (fgets(line, sizeof(line), inputFile) != NULL) {
        if (strcmp(trim_whitespace(line), "material(){\n") == 0) {
            materialCount++;
        }
        if (strcmp(trim_whitespace(line), "light(){\n") == 0) {
            lightCount++;
        }
        if (strcmp(trim_whitespace(line), "sphere(){\n") == 0) {
            sphereCount++;
        }
        if (strcmp(trim_whitespace(line), "poly(){\n") == 0) {
            polyCount++;
        }
		if (strcmp(trim_whitespace(line), "OBJ(){\n") == 0) {
			objCount++;
		}
    }
    fclose(inputFile);
    scene->materials = (material *)calloc(materialCount, sizeof(material));
    scene->lights = (lightSphere *)calloc(lightCount, sizeof(lightSphere));
    scene->spheres = (sphere*)calloc(sphereCount, sizeof(sphere));
    scene->polys = (poly*)calloc(polyCount, sizeof(poly));
    
    scene->materialAmount = materialCount;
    scene->lightAmount = lightCount;
    scene->sphereAmount = sphereCount;
    scene->polygonAmount = polyCount;
	scene->objCount = objCount;
    return 0;
}

//Removes tabs and spaces from a char byte array, terminates it and returns it.
char *trim_whitespace(char *inputLine) {
    int i, j;
    char *outputLine = inputLine;
    for (i = 0, j = 0; i < strlen(inputLine); i++, j++) {
        if (inputLine[i] == ' ') { //Space
            j--;
        } else if (inputLine[i] == '\t') { //Tab
            j--;
        } else {
            outputLine[j] = inputLine[i];
        }
    }
    //Add null termination byte
    outputLine[j] = '\0';
    return outputLine;
}
