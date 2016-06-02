//
//  scene.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "scene.h"

#define TOKEN_DEBUG_ENABLED false

//Prototypes
//Trims spaces and tabs from a char array
char *trim_whitespace(char *inputLine);
//Parses a scene file and allocates memory accordingly
int allocMemory(world *scene, char *inputFileName);

int buildScene(world *scene, char *inputFileName) {
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
						scene->camera.viewPerspective.projectionType = conic;
					} else if (strncmp(savePointer, "ortho", 5) == 0){
						scene->camera.viewPerspective.projectionType = ortho;
					}
				}
                
                if (strncmp(trim_whitespace(line), "fileFormat", 10) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorCamera);
                    
                    if (strncmp(savePointer, "bmp", 3) == 0) {
                        scene->camera.outputFileType = bmp;
                    } else if (strncmp(savePointer, "ppm", 3) == 0) {
                        scene->camera.outputFileType = ppm;
                    }else if (strncmp(savePointer, "png", 3) == 0) {
                        scene->camera.outputFileType = png;
                    }
                }
                //TODO: Here
                if (strncmp(trim_whitespace(line), "forceSingleCore", 15) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
                    if (token == NULL)
                        logHandler(sceneParseErrorCamera);
                    if (strncmp(savePointer, "true", 4) == 0) {
                        //scene->camera.antialiased = true;
                    } else if (strncmp(savePointer, "false", 5) == 0) {
                        //scene->camera.antialiased = false;
                    }
                }
				
				if (strncmp(trim_whitespace(line), "FOV", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					float FOV = strtof(savePointer, NULL);
					scene->camera.viewPerspective.FOV = FOV;
				}
				
				if (strncmp(trim_whitespace(line), "contrast", 8) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					float contrast = strtof(savePointer, NULL);
					scene->camera.contrast = contrast;
				}
				
				if (strncmp(trim_whitespace(line), "antialiased", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					if (strncmp(savePointer, "true", 4) == 0) {
						scene->camera.antialiased = true;
					} else if (strncmp(savePointer, "false", 5) == 0) {
						scene->camera.antialiased = false;
					}
				}
				
				if (strncmp(trim_whitespace(line), "supersampling", 13) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int supersampling = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.supersampling = supersampling;
				}
				
				if (strncmp(trim_whitespace(line), "bounces", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int bounces = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.bounces = bounces;
				}
				
				if (strncmp(trim_whitespace(line), "posX", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.pos.x = posX;
				}
				
				if (strncmp(trim_whitespace(line), "posY", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.pos.y = posY;
				}
				
				if (strncmp(trim_whitespace(line), "posZ", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posZ = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.pos.z = posZ;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtX", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.lookAt.x = lookAtX;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtY", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.lookAt.y = lookAtY;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtZ", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtZ = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.lookAt.z = lookAtZ;
				}
				
				if (strncmp(trim_whitespace(line), "resolutionX", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int resolutionX = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.width = resolutionX;
				}
				
				if (strncmp(trim_whitespace(line), "resolutionY", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int resolutionY = (int)strtol(savePointer, (char**)NULL, 10);
					scene->camera.height = resolutionY;
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
}

int loadOBJ(world *scene, char *inputFileName) {
    obj_scene_data sceneData;
    if (!parse_obj_scene(&sceneData, "../output/test.obj")) {
        return 0;
    }
    scene->lightAmount = sceneData.light_point_count;
    scene->sphereAmount = sceneData.sphere_count;
    
    return 0;
}

int allocMemory(world *scene, char *inputFileName) {
    int materialCount = 0, lightCount = 0, polyCount = 0, sphereCount = 0;
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
    }
    fclose(inputFile);
    scene->materials = (material *)calloc(materialCount, sizeof(material));
    scene->lights = (lightSphere *)calloc(lightCount, sizeof(lightSphere));
    scene->spheres = (sphereObject*)calloc(sphereCount, sizeof(sphereObject));
    scene->polys = (polygonObject*)calloc(polyCount, sizeof(polygonObject));
    
    scene->materialAmount = materialCount;
    scene->lightAmount = lightCount;
    scene->sphereAmount = sphereCount;
    scene->polygonAmount = polyCount;
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