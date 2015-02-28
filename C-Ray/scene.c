//
//  scene.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "scene.h"

//TODO: Turn this into a proper tokenizer
int buildScene(bool randomGenerator, world *scene) {
	if (!randomGenerator) {
		printf("Building scene\n");
		
		scene->width = kImgWidth;
		scene->height = kImgHeight;
		
		scene->viewPerspective.projectionType = conic;
		scene->viewPerspective.FOV = 90.0f;
		
		scene->ambientColor = (color * )calloc(3, sizeof(color));
		scene->ambientColor->red =   0.41;
		scene->ambientColor->green = 0.41;
		scene->ambientColor->blue =  0.41;
		
		//define materials to an array
		scene->materialAmount = 6;
		
		scene->materials = (material *)calloc(scene->materialAmount, sizeof(material));
		scene->materials[0].diffuse.red = 1;
		scene->materials[0].diffuse.green = 0;
		scene->materials[0].diffuse.blue = 0;
		scene->materials[0].reflectivity = 0.2;
		
		scene->materials[1].diffuse.red = 0;
		scene->materials[1].diffuse.green = 1;
		scene->materials[1].diffuse.blue = 0;
		scene->materials[1].reflectivity = 0.5;
		
		scene->materials[2].diffuse.red = 0;
		scene->materials[2].diffuse.green = 0;
		scene->materials[2].diffuse.blue = 1;
		scene->materials[2].reflectivity = 0.2;
		
		scene->materials[3].diffuse.red = 0.3;
		scene->materials[3].diffuse.green = 0.3;
		scene->materials[3].diffuse.blue = 0.3;
		scene->materials[3].reflectivity = 1;
		
		scene->materials[4].diffuse.red = 0;
		scene->materials[4].diffuse.green = 0.517647;
		scene->materials[4].diffuse.blue = 1;
		scene->materials[4].reflectivity = 1;
		
		scene->materials[5].diffuse.red = 0.3;
		scene->materials[5].diffuse.green = 0.3;
		scene->materials[5].diffuse.blue = 0.3;
		scene->materials[5].reflectivity = 0.5;
		
		//Define polygons to an array
		scene->polygonAmount = 4;
		
		scene->polys = (polygonObject *)calloc(scene->polygonAmount, sizeof(polygonObject));
		//Bottom plane
		scene->polys[0].v1.x = 200;
		scene->polys[0].v1.y = 50;
		scene->polys[0].v1.z = 0;
		
		scene->polys[0].v2.x = kImgWidth-200;
		scene->polys[0].v2.y = 50;
		scene->polys[0].v2.z = 0;
		
		scene->polys[0].v3.x = 400;
		scene->polys[0].v3.y = kImgHeight/2;
		scene->polys[0].v3.z = 2000;
		scene->polys[0].material = 3;
		
		scene->polys[1].v1.x = kImgWidth-200;
		scene->polys[1].v1.y = 50;
		scene->polys[1].v1.z = 0;
		
		scene->polys[1].v2.x = kImgWidth-400;
		scene->polys[1].v2.y = kImgHeight/2;
		scene->polys[1].v2.z = 2000;
		
		scene->polys[1].v3.x = 400;
		scene->polys[1].v3.y = kImgHeight/2;
		scene->polys[1].v3.z = 2000;
		scene->polys[1].material = 3;
		
		//Background plane
		//First poly
		//bottom left
		scene->polys[2].v1.x = 400;
		scene->polys[2].v1.y = kImgHeight/2;
		scene->polys[2].v1.z = 2000;
		//Bottom right
		scene->polys[2].v2.x = kImgWidth-400;
		scene->polys[2].v2.y = kImgHeight/2;
		scene->polys[2].v2.z = 2000;
		//top left
		scene->polys[2].v3.x = 400;
		scene->polys[2].v3.y = kImgHeight;
		scene->polys[2].v3.z = 1900;
		scene->polys[2].material = 5;
		//Second poly
		//top right
		scene->polys[3].v1.x = kImgWidth-400;
		scene->polys[3].v1.y = kImgHeight;
		scene->polys[3].v1.z = 1900;
		//top left
		scene->polys[3].v2.x = 400;
		scene->polys[3].v2.y = kImgHeight;
		scene->polys[3].v2.z = 1900;
		//bottom right
		scene->polys[3].v3.x = kImgWidth-400;
		scene->polys[3].v3.y = kImgHeight/2;
		scene->polys[3].v3.z = 2000;
		scene->polys[3].material = 5;
		
		//define spheres to an array
		//Red sphere
		scene->sphereAmount = 4;
		
		scene->spheres = (sphereObject *)calloc(scene->sphereAmount, sizeof(sphereObject));
		scene->spheres[0].pos.x = 400;
		scene->spheres[0].pos.y = 260;
		scene->spheres[0].pos.z = 0;
		scene->spheres[0].radius = 200;
		scene->spheres[0].material = 0;
		
		//green sphere
		scene->spheres[1].pos.x = 650;
		scene->spheres[1].pos.y = 630;
		scene->spheres[1].pos.z = 1750;
		scene->spheres[1].radius = 150;
		scene->spheres[1].material = 1;
		
		//blue sphere
		scene->spheres[2].pos.x = 1300;
		scene->spheres[2].pos.y = 520;
		scene->spheres[2].pos.z = 1000;
		scene->spheres[2].radius = 220;
		scene->spheres[2].material = 2;
		
		//grey sphere
		scene->spheres[3].pos.x = 970;
		scene->spheres[3].pos.y = 250;
		scene->spheres[3].pos.z = 400;
		scene->spheres[3].radius = 100;
		scene->spheres[3].material = 3;
		
		//Define lights to an array
		scene->lightAmount = 1;
		
		scene->lights = (lightSource *)calloc(scene->lightAmount, sizeof(lightSource));
		scene->lights[0].pos.x = kImgWidth/2;
		scene->lights[0].pos.y = 1080-100;
		scene->lights[0].pos.z = 0;
		scene->lights[0].intensity.red = 0.9;
		scene->lights[0].intensity.green = 0.9;
		scene->lights[0].intensity.blue = 0.9;
		scene->lights[0].radius = 1.0;
		
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
		
		scene->viewPerspective.projectionType = conic;
		scene->viewPerspective.FOV = 90.0f;
		
		scene->ambientColor = (color * )calloc(3, sizeof(color));
		scene->ambientColor->red =   0.41;
		scene->ambientColor->green = 0.41;
		scene->ambientColor->blue =  0.41;
		
		scene->polygonAmount = -1;
		scene->height = kImgHeight;
		scene->width = kImgWidth;
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
}