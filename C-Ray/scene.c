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

#define TOKEN_DEBUG_ENABLED true

//TODO: Turn this into a proper tokenizer

//Prototypes
char *trim_whitespace(char *inputLine);
char *find_in_char(char *inputLine, char *token);

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
    
    int materialsCount = 0, spheresCount = 0, polyCount = 0, lightsCount = 0;
	
	//Suppress obnoxious warnings
	if (delimComma && delimPoint && openBlock && closeBlock && subtoken && savePointer2 && savePointer1 && token && materialsCount && spheresCount && polyCount && lightsCount) {
		//Nope
	}
    
    char line[255];
    
    while (fgets(line, sizeof(line), inputFile) != NULL) {
        //Remove whitespace from all lines
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
					int amount = (int)strtol(savePointer1, (char**) NULL, 10);
					scene->sphereAmount = amount;
					scene->spheres = (sphereObject *)calloc(scene->sphereAmount, sizeof(sphereObject));
				}
				
				if (strncmp(trim_whitespace(line), "polygonAmount", 13) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					int amount = (int)strtol(savePointer1, (char**) NULL, 10);
					scene->polygonAmount = amount;
					scene->polys = (polygonObject *)calloc(scene->polygonAmount, sizeof(polygonObject));
				}
				
				if (strncmp(trim_whitespace(line), "materialAmount", 14) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					int amount = (int)strtol(savePointer1, (char**) NULL, 10);
					scene->materialAmount = amount;
					scene->materials = (material *)calloc(scene->materialAmount, sizeof(material));
				}
				
				if (strncmp(trim_whitespace(line), "lightAmount", 11) == 0) {
					token = strtok_r(trim_whitespace(line), delimEquals, &savePointer1);
					if (token == NULL)
						logHandler(sceneParseErrorScene);
					int amount = (int)strtol(savePointer1, (char**) NULL, 10);
					scene->lightAmount = amount;
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
				
			}
		}
		
		if (strcmp(trim_whitespace(line), "material(){\n") == 0) {
			printf("Found material\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorMaterial);
				
			}
			
		}
		
		if (strcmp(trim_whitespace(line), "light(){\n") == 0) {
			printf("Found light\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorLight);
				
			}
		}
		
		if (strcmp(trim_whitespace(line), "sphere(){\n") == 0) {
			printf("Found sphere\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorSphere);
				
			}
		}
		if (strcmp(trim_whitespace(line), "poly(){\n") == 0) {
			printf("Found poly\n");
			while (trim_whitespace(line)[0] != *closeBlock) {
				error = fgets(trim_whitespace(line), sizeof(line), inputFile);
				if (!error)
					logHandler(sceneParseErrorPoly);
				
			}
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

/*char find_in_char(char *inputLine) {
    int i, j;
    char *outputLine = inputLine;
    for (i = 0, j = 0; i < strlen(inputLine); i++, j++) {
        if (true) {
            <#statements#>
        }
    }
}*/

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