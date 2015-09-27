//
//  scene.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "scene.h"
#include "errorhandler.h"
#include <string.h>

#define TOKEN_DEBUG_ENABLED false

//Prototypes
char *trim_whitespace(char *inputLine);

int buildScene(world *scene, char *inputFileName) {
    printf("\nStarting C-ray Scene Parser 0.1\n");
    FILE *inputFile = fopen(inputFileName, "r");
    if (!inputFile)
        return -1;
    
    char *error = NULL;
    char *delimEquals = "=";
    char *delimComma = ",", *delimPoint = ".";
    char *openBlock = "{", *closeBlock = "}";
    
    char *token, *subtoken;
	char *savePointer1, savePointer2;
	
	int materialIndex = 0, sphereIndex = 0, polyIndex = 0, lightIndex = 0;
	
	//Suppress obnoxious warnings
	if (delimPoint && openBlock && subtoken && savePointer2 && token && materialIndex && sphereIndex && polyIndex && lightIndex) {
		//Nope
	}
    
    char line[255];
    
    while (fgets(line, sizeof(line), inputFile) != NULL) {
        printf("%s",trim_whitespace(line));
        if (*trim_whitespace(line) != '\n') {
            printf("\n");
        }
        
        //Discard comments
        if (trim_whitespace(line)[0] == '#') {
            printf("Found comment, ignoring.\n");
        }
        if (strcmp(trim_whitespace(line), "scene(){\n") == 0) {
            printf("Found scene\n");
            while (trim_whitespace(line)[0] != *closeBlock) {
                error = fgets(trim_whitespace(line), sizeof(line), inputFile);
                if (!error)
                    logHandler(sceneParseErrorSphere);
				
				if (strncmp(trim_whitespace(line), "ambientColor", 12) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					scene->ambientColor = (color * )calloc(3, sizeof(color));
					token = strtok_r(NULL, delimComma, &savePointer1);  //Red
					float rgbValue = strtof(token, NULL);
					scene->ambientColor->red = rgbValue;
					token = strtok_r(NULL, delimComma, &savePointer1);  //Green
					rgbValue = strtof(token, NULL);
					scene->ambientColor->green = rgbValue;
					token = strtok_r(NULL, delimComma, &savePointer1);  //Blue
					rgbValue = strtof(token, NULL);
					scene->ambientColor->blue = rgbValue;
				}
				
				if (strncmp(trim_whitespace(line), "sphereAmount", 12) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					int sphereAmount = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->sphereAmount = sphereAmount;
					scene->spheres = (sphereObject *)calloc(scene->sphereAmount, sizeof(sphereObject));
				}
				
				if (strncmp(trim_whitespace(line), "polygonAmount", 13) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					int polygonAmount = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->polygonAmount = polygonAmount;
					scene->polys = (polygonObject *)calloc(scene->polygonAmount, sizeof(polygonObject));
				}
				
				if (strncmp(trim_whitespace(line), "materialAmount", 14) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					int materialAmount = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->materialAmount = materialAmount;
					scene->materials = (material *)calloc(scene->materialAmount, sizeof(material));
				}
				
				if (strncmp(trim_whitespace(line), "lightAmount", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					int lightAmount = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->lightAmount = lightAmount;
					scene->lights = (lightSource *)calloc(scene->lightAmount, sizeof(lightSource));
				}
            }
        }
		
		if (strcmp(trim_whitespace(line), "camera(){\n") == 0) {
			printf("Found camera\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorCamera);
				
				if (strncmp(trim_whitespace(line), "perspective", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					
					if (strncmp(savePointer1, "conic", 5) == 0) {
						scene->camera.viewPerspective.projectionType = conic;
					} else if (strncmp(savePointer1, "ortho", 5) == 0){
						scene->camera.viewPerspective.projectionType = ortho;
					}
				}
				
				if (strncmp(trim_whitespace(line), "FOV", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					float FOV = strtof(savePointer1, NULL);
					scene->camera.viewPerspective.FOV = FOV;
				}
				
				if (strncmp(trim_whitespace(line), "contrast", 8) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					float contrast = strtof(savePointer1, NULL);
					scene->camera.contrast = contrast;
				}
				
				if (strncmp(trim_whitespace(line), "antialiased", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					if (strncmp(savePointer1, "true", 4) == 0) {
						scene->camera.antialiased = true;
					} else if (strncmp(savePointer1, "false", 5) == 0) {
						scene->camera.antialiased = false;
					}
				}
				
				if (strncmp(trim_whitespace(line), "supersampling", 13) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int supersampling = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.supersampling = supersampling;
				}
				
				if (strncmp(trim_whitespace(line), "bounces", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int bounces = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.bounces = bounces;
				}
				
				if (strncmp(trim_whitespace(line), "posX", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posX = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.pos.x = posX;
				}
				
				if (strncmp(trim_whitespace(line), "posY", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posY = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.pos.y = posY;
				}
				
				if (strncmp(trim_whitespace(line), "posZ", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int posZ = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.pos.z = posZ;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtX", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtX = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.lookAt.x = lookAtX;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtY", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtY = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.lookAt.x = lookAtY;
				}
				
				if (strncmp(trim_whitespace(line), "lookAtZ", 7) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int lookAtZ = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.lookAt.x = lookAtZ;
				}
				
				if (strncmp(trim_whitespace(line), "resolutionX", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int resolutionX = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.width = resolutionX;
				}
				
				if (strncmp(trim_whitespace(line), "resolutionY", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorCamera);
					int resolutionY = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->camera.height = resolutionY;
				}
			}
		}
		
		if (strcmp(trim_whitespace(line), "material(){\n") == 0) {
			printf("Found material\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorMaterial);
				
				if (strncmp(trim_whitespace(line), "red", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float red = strtof(savePointer1, NULL);
					scene->materials[materialIndex].diffuse.red = red;
				}
				
				if (strncmp(trim_whitespace(line), "green", 5) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float green = strtof(savePointer1, NULL);
					scene->materials[materialIndex].diffuse.green = green;
				}
				
				if (strncmp(trim_whitespace(line), "blue", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float blue = strtof(savePointer1, NULL);
					scene->materials[materialIndex].diffuse.blue = blue;
				}
				
				if (strncmp(trim_whitespace(line), "reflectivity", 12) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorMaterial);
					float reflectivity = strtof(savePointer1, NULL);
					scene->materials[materialIndex].reflectivity = reflectivity;
				}
			}
			materialIndex++;
		}
		
		if (strcmp(trim_whitespace(line), "light(){\n") == 0) {
			printf("Found light\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorLight);
				
				if (strncmp(trim_whitespace(line), "posX", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					int posX = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->lights[lightIndex].pos.x = posX;
				}
				
				if (strncmp(trim_whitespace(line), "posY", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					int posY = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->lights[lightIndex].pos.x = posY;
				}
				
				if (strncmp(trim_whitespace(line), "posZ", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					int posZ = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->lights[lightIndex].pos.x = posZ;
				}
				
				if (strncmp(trim_whitespace(line), "red", 3) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float red = strtof(savePointer1, NULL);
					scene->lights[lightIndex].intensity.red = red;
				}
				
				if (strncmp(trim_whitespace(line), "green", 5) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float green = strtof(savePointer1, NULL);
					scene->lights[lightIndex].intensity.green = green;
				}
				
				if (strncmp(trim_whitespace(line), "blue", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float blue = strtof(savePointer1, NULL);
					scene->lights[lightIndex].intensity.blue = blue;
				}
				
				if (strncmp(trim_whitespace(line), "radius", 6) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorLight);
					float radius = strtof(savePointer1, NULL);
					scene->lights[lightIndex].radius = radius;
				}
			}
			lightIndex++;
		}
		
		if (strcmp(trim_whitespace(line), "sphere(){\n") == 0) {
			printf("Found sphere\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorSphere);
				
				if (strncmp(trim_whitespace(line), "posX", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int posX = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->spheres[sphereIndex].pos.x = posX;
				}
				
				if (strncmp(trim_whitespace(line), "posY", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int posY = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->spheres[sphereIndex].pos.y = posY;
				}
				
				if (strncmp(trim_whitespace(line), "posZ", 4) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int posZ = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->spheres[sphereIndex].pos.z = posZ;
				}
				
				if (strncmp(trim_whitespace(line), "radius", 6) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					float radius = strtof(savePointer1, NULL);
					scene->spheres[sphereIndex].radius = radius;
				}
				
				if (strncmp(trim_whitespace(line), "material", 8) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorSphere);
					int material = (int)strtol(savePointer1, (char**)NULL, 10);
					scene->spheres[sphereIndex].material = material;
				}
			}
			sphereIndex++;
		}
		if (strcmp(trim_whitespace(line), "poly(){\n") == 0) {
			printf("Found poly\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorPoly);
                
                if (strncmp(trim_whitespace(line), "v1X", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v1X = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v1.x = v1X;
                }
                
                if (strncmp(trim_whitespace(line), "v1Y", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v1Y = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v1.y = v1Y;
                }
                
                if (strncmp(trim_whitespace(line), "v1Z", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v1Z = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v1.z = v1Z;
                }
                
                
                if (strncmp(trim_whitespace(line), "v2X", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v2X = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v2.x = v2X;
                }
                
                if (strncmp(trim_whitespace(line), "v2Y", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v2Y = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v2.y = v2Y;
                }
                
                if (strncmp(trim_whitespace(line), "v2Z", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v2Z = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v2.z = v2Z;
                }
                
                
                if (strncmp(trim_whitespace(line), "v3X", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v3X = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v3.x = v3X;
                }
                
                if (strncmp(trim_whitespace(line), "v3Y", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v3Y = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v3.y = v3Y;
                }
                
                if (strncmp(trim_whitespace(line), "v3Z", 3) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int v3Z = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].v3.z = v3Z;
                }
                
                if (strncmp(trim_whitespace(line), "material", 8) == 0) {
                    token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
                    if (token == NULL)
                        logHandler(sceneParseErrorPoly);
                    int material = (int)strtol(savePointer1, (char**)NULL, 10);
                    scene->polys[polyIndex].material = material;
                }
			}
			polyIndex++;
		}
    }
    fclose(inputFile);
    printf("\n");
    
    if (TOKEN_DEBUG_ENABLED) {
        return 4; //Debug mode - Won't render anything
    } else {
        return 0;
    }
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

/*int buildScene(bool randomGenerator, world *scene) {
	if (!randomGenerator) {
		printf("Building scene\n");
		
		//Final render resolution
		scene->camera.width = kImgWidth;
		scene->camera.height = kImgHeight;
		
		scene->camera.pos.x = 940;
		scene->camera.pos.y = 480;
		scene->camera.pos.z = 0;
		
		scene->camera.lookAt.x = 0;
		scene->camera.lookAt.y = 0;
		scene->camera.lookAt.z = 1;
		
		scene->camera.viewPerspective.projectionType = conic;
        scene->camera.antialiased = true;
        scene->camera.supersampling = 10;
		scene->camera.viewPerspective.FOV = 80.0f;
		
		scene->ambientColor = (color * )calloc(3, sizeof(color));
		scene->ambientColor->red =   0.41;
		scene->ambientColor->green = 0.41;
		scene->ambientColor->blue =  0.41;
		
		//define materials to an array
		scene->materialAmount = 8;
		
		scene->materials = (material *)calloc(scene->materialAmount, sizeof(material));
		//Matte Red
		scene->materials[0].diffuse.red = 0.8;
		scene->materials[0].diffuse.green = 0.1;
		scene->materials[0].diffuse.blue = 0.1;
		scene->materials[0].reflectivity = 0;
		
		//Matte green
		scene->materials[1].diffuse.red = 0.1;
		scene->materials[1].diffuse.green = 0.8;
		scene->materials[1].diffuse.blue = 0.1;
		scene->materials[1].reflectivity = 0;
		
		//Matte blue
		scene->materials[2].diffuse.red = 0.1;
		scene->materials[2].diffuse.green = 0.1;
		scene->materials[2].diffuse.blue = 0.8;
		scene->materials[2].reflectivity = 0;
		
		//Matte Gray
		scene->materials[3].diffuse.red = 0.8;
		scene->materials[3].diffuse.green = 0.8;
		scene->materials[3].diffuse.blue = 0.8;
		scene->materials[3].reflectivity = 0;
		
		//Blue-ish
		scene->materials[4].diffuse.red = 0;
		scene->materials[4].diffuse.green = 0.517647;
		scene->materials[4].diffuse.blue = 1;
		scene->materials[4].reflectivity = 1;
		
		//Reflective gray
		scene->materials[5].diffuse.red = 0.3;
		scene->materials[5].diffuse.green = 0.3;
		scene->materials[5].diffuse.blue = 0.3;
		scene->materials[5].reflectivity = 1;
        
        //Reflective Red
        scene->materials[6].diffuse.red = 0.3;
        scene->materials[6].diffuse.green = 0;
        scene->materials[6].diffuse.blue = 0;
        scene->materials[6].reflectivity = 1;
        //Reflective Green
        scene->materials[7].diffuse.red = 0;
        scene->materials[7].diffuse.green = 0.3;
        scene->materials[7].diffuse.blue = 0;
        scene->materials[7].reflectivity = 1;
        //Reflective Blue
        scene->materials[8].diffuse.red = 0;
        scene->materials[8].diffuse.green = 0;
        scene->materials[8].diffuse.blue = 0.3;
        scene->materials[8].reflectivity = 1;
        
		//Define polygons to an array
		//scene->polygonAmount = 11;
		scene->polygonAmount = 11;
		
		scene->polys = (polygonObject *)calloc(scene->polygonAmount, sizeof(polygonObject));
		//Floor plane
		//First poly (Bottom left)
		//Bottom left
		scene->polys[0].v1.x = 200;
		scene->polys[0].v1.y = 300;
		scene->polys[0].v1.z = 0;
		//Bottom right
		scene->polys[0].v2.x = 1720;
		scene->polys[0].v2.y = 300;
		scene->polys[0].v2.z = 0;
		//Top left
		scene->polys[0].v3.x = 200;
		scene->polys[0].v3.y = 300;
		scene->polys[0].v3.z = 2000;
		scene->polys[0].material = 3;
		//Second poly (Top right)
		//Bottom right
		scene->polys[1].v1.x = 1720;
		scene->polys[1].v1.y = 300;
		scene->polys[1].v1.z = 0;
		//Top right
		scene->polys[1].v2.x = 1720;
		scene->polys[1].v2.y = 300;
		scene->polys[1].v2.z = 2000;
		//Top left
		scene->polys[1].v3.x = 200;
		scene->polys[1].v3.y = 300;
		scene->polys[1].v3.z = 2000;
		scene->polys[1].material = 3;
		
		//Background plane (Red)
		//First poly
		//bottom left
		scene->polys[2].v1.x = 200;
		scene->polys[2].v1.y = 300;
		scene->polys[2].v1.z = 2000;
		//Bottom right
		scene->polys[2].v2.x = 1720;
		scene->polys[2].v2.y = 300;
		scene->polys[2].v2.z = 2000;
		//top left
		scene->polys[2].v3.x = 200;
		scene->polys[2].v3.y = 840;
		scene->polys[2].v3.z = 2000;
		scene->polys[2].material = 0;
		//Second poly
		//top right
		scene->polys[3].v1.x = 1720;
		scene->polys[3].v1.y = 840;
		scene->polys[3].v1.z = 2000;
		//top left
		scene->polys[3].v2.x = 200;
		scene->polys[3].v2.y = 840;
		scene->polys[3].v2.z = 2000;
		//bottom right
		scene->polys[3].v3.x = 1720;
		scene->polys[3].v3.y = 300;
		scene->polys[3].v3.z = 2000;
		scene->polys[3].material = 0;
		
		//Roof plane
		//First poly (Left)
		//Bottom left
		scene->polys[4].v1.x = 200;
		scene->polys[4].v1.y = 840;
		scene->polys[4].v1.z = 0;
		//Bottom right
		scene->polys[4].v2.x = 1720;
		scene->polys[4].v2.y = 840;
		scene->polys[4].v2.z = 0;
		//Top left
		scene->polys[4].v3.x = 200;
		scene->polys[4].v3.y = 840;
		scene->polys[4].v3.z = 2000;
		scene->polys[4].material = 3;
		//Second poly (Right)
		//Bottom right
		scene->polys[5].v1.x = 1720;
		scene->polys[5].v1.y = 840;
		scene->polys[5].v1.z = 0;
		//Top right
		scene->polys[5].v2.x = 1720;
		scene->polys[5].v2.y = 840;
		scene->polys[5].v2.z = 2000;
		//Top left
		scene->polys[5].v3.x = 200;
		scene->polys[5].v3.y = 840;
		scene->polys[5].v3.z = 2000;
		scene->polys[5].material = 3;
		
		//Right wall
		//First poly (Bottom left corner)
		//Bottom left
		scene->polys[6].v1.x = 1720;
		scene->polys[6].v1.y = 300;
		scene->polys[6].v1.z = 2000;
		//Top left
		scene->polys[6].v2.x = 1720;
		scene->polys[6].v2.y = 840;
		scene->polys[6].v2.z = 2000;
		//Top left
		scene->polys[6].v3.x = 1720;
		scene->polys[6].v3.y = 300;
		scene->polys[6].v3.z = 0;
		scene->polys[6].material = 2;
		//Second poly (Top right corner)
		//Top left
		scene->polys[7].v1.x = 1720;
		scene->polys[7].v1.y = 840;
		scene->polys[7].v1.z = 2000;
		//Top right
		scene->polys[7].v2.x = 1720;
		scene->polys[7].v2.y = 840;
		scene->polys[7].v2.z = 0;
		//Bottom right
		scene->polys[7].v3.x = 1720;
		scene->polys[7].v3.y = 300;
		scene->polys[7].v3.z = 0;
		scene->polys[7].material = 2;
		
		//Left wall
		//First poly (Bottom right corner)
		//Bottom left
		scene->polys[8].v1.x = 200;
		scene->polys[8].v1.y = 300;
		scene->polys[8].v1.z = 2000;
		//Top left
		scene->polys[8].v2.x = 200;
		scene->polys[8].v2.y = 840;
		scene->polys[8].v2.z = 2000;
		//Top left
		scene->polys[8].v3.x = 200;
		scene->polys[8].v3.y = 300;
		scene->polys[8].v3.z = 0;
		scene->polys[8].material = 1;
		//Second poly (Top left corner)
		//Top left
		scene->polys[9].v1.x = 200;
		scene->polys[9].v1.y = 840;
		scene->polys[9].v1.z = 2000;
		//Top right
		scene->polys[9].v2.x = 200;
		scene->polys[9].v2.y = 840;
		scene->polys[9].v2.z = 0;
		//Bottom right
		scene->polys[9].v3.x = 200;
		scene->polys[9].v3.y = 300;
		scene->polys[9].v3.z = 0;
		scene->polys[9].material = 1;
		
		//Shadow test polygon
		scene->polys[10].v1.x = 1000;
		scene->polys[10].v1.y = 450;
		scene->polys[10].v1.z = 1400;
		
		scene->polys[10].v2.x = 1300;
		scene->polys[10].v2.y = 700;
		scene->polys[10].v2.z = 1400;
		
		scene->polys[10].v3.x = 1000;
		scene->polys[10].v3.y = 700;
		scene->polys[10].v3.z = 1600;
		scene->polys[10].material = 4;
		
		//define spheres to an array
		//Red sphere
		scene->sphereAmount = 4;
		
		scene->spheres = (sphereObject *)calloc(scene->sphereAmount, sizeof(sphereObject));
		
		//Gray sphere
		scene->spheres[0].pos.x = 650;
		scene->spheres[0].pos.y = 450;
		scene->spheres[0].pos.z = 1650;
		scene->spheres[0].radius = 150;
		scene->spheres[0].material = 5;
		
		//Red sphere
        scene->spheres[1].pos.x = 750;
		scene->spheres[1].pos.y = 450;
		scene->spheres[1].pos.z = 1000;
		scene->spheres[1].radius = 50;
		scene->spheres[1].material = 6;
		
		//Green sphere
		scene->spheres[2].pos.x = 850;
		scene->spheres[2].pos.y = 450;
		scene->spheres[2].pos.z = 400;
		scene->spheres[2].radius = 50;
		scene->spheres[2].material = 7;
        
        //Blue sphere
        scene->spheres[3].pos.x = 1100;
        scene->spheres[3].pos.y = 450;
        scene->spheres[3].pos.z = 800;
        scene->spheres[3].radius = 50;
        scene->spheres[3].material = 8;
		
		//Define lights to an array
		scene->lightAmount = 2;
		
		scene->lights = (lightSource *)calloc(scene->lightAmount, sizeof(lightSource));
		scene->lights[0].pos.x = 960;
		scene->lights[0].pos.y = 400;
		scene->lights[0].pos.z = 0;
		scene->lights[0].intensity.red = 1;
		scene->lights[0].intensity.green = 1;
		scene->lights[0].intensity.blue = 1;
		scene->lights[0].radius = 1.0;
		
		scene->lights[1].pos.x = 960;
		scene->lights[1].pos.y = 500;
		scene->lights[1].pos.z = 0;
		scene->lights[1].intensity.red = 0.8;
		scene->lights[1].intensity.green = 0.8;
		scene->lights[1].intensity.blue = 0.8;
		return 0;
	} else {
		//Define random scene
		printf("Building random scene");
		int amount = rand()%50+50;
		printf("Spheres: %i\n",amount);
		int i;
		
		scene->lightAmount = 1;
		
		scene->lights = (lightSource *)calloc(scene->lightAmount, sizeof(lightSource));
		scene->lights[0].pos.x = kImgWidth/2;
		scene->lights[0].pos.y = 1080-100;
		scene->lights[0].pos.z = 0;
		scene->lights[0].intensity.red = 0.9;
		scene->lights[0].intensity.green = 0.9;
		scene->lights[0].intensity.blue = 0.9;
		scene->lights[0].radius = 1.0;
		
		//scene->camera = (camera *)calloc(scene->camera, sizeof(camera));
		scene->camera.viewPerspective.projectionType = conic;
		scene->camera.viewPerspective.FOV = 90.0f;
		scene->camera.height = kImgHeight;
		scene->camera.width = kImgWidth;
		
		scene->ambientColor = (color * )calloc(3, sizeof(color));
		scene->ambientColor->red =   0.41;
		scene->ambientColor->green = 0.41;
		scene->ambientColor->blue =  0.41;
		
		scene->polygonAmount = -1;
		scene->materialAmount = i;
		scene->materials = (material *)calloc(scene->materialAmount, sizeof(material));
		scene->sphereAmount = i;
		scene->spheres = (sphereObject *)calloc(scene->sphereAmount, sizeof(sphereObject));
		
		for (i = 0; i < amount; i++) {
			scene->materials[i].diffuse.red = randRange(0,1);
			scene->materials[i].diffuse.green = randRange(0,1);
			scene->materials[i].diffuse.blue = randRange(0,1);
			scene->materials[i].reflectivity = randRange(0,1);
			
			scene->spheres[i].pos.x = rand()%kImgWidth;
			scene->spheres[i].pos.y = rand()%kImgWidth;
			scene->spheres[i].pos.z = rand()&4000-2000;
			scene->spheres[i].radius = rand()%100+50;
			scene->spheres[i].material = i;
		}
		return 0;
	}
}*/