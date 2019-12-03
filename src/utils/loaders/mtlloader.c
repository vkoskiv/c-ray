//
//  mtlloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

// C-ray MTL file parser

#include "../../includes.h"
#include "mtlloader.h"

#include "../../datatypes/material.h"
#include "../../utils/logging.h"
#include "../../utils/filehandler.h" //for copyString FIXME

// Parse a list of materials and return an array of materials.
// mtlCount is the amount of materials loaded.
struct material *parseMTLFile(char *filePath, int *mtlCount) {
	struct material *newMaterials = NULL;
	
	int count = 0;
	int linenum = 0;
	char *token;
	char currLine[500];
	FILE *fileStream;
	struct material *currMat = NULL;
	bool matOpen = false;
	
	fileStream = fopen(filePath, "r");
	if (fileStream == 0) {
		logr(warning, "Material not found at %s\n", filePath);
		return NULL;
	}
	
	while (fgets(currLine, 500, fileStream)) {
		token = strtok(currLine, " \t\n\r");
		linenum++;
		
		if (token == NULL || stringEquals(token, "#")) {
			//Skip comments starting with #
			continue;
		} else if (stringEquals(token, "newmtl")) {
			//New material is created
			count++;
			newMaterials = realloc(newMaterials, count * sizeof(struct material));
			currMat = &newMaterials[count-1];
			currMat->name = calloc(CRAY_MATERIAL_NAME_SIZE, sizeof(char));
			currMat->textureFilePath = calloc(CRAY_MESH_FILENAME_LENGTH, sizeof(char));
			strncpy(currMat->name, strtok(NULL, " \t"), CRAY_MATERIAL_NAME_SIZE);
			matOpen = true;
		} else if (stringEquals(token, "Ka") && matOpen) {
			//Ambient color
			currMat->ambient.red = atof(strtok(NULL, " \t"));
			currMat->ambient.green = atof(strtok(NULL, " \t"));
			currMat->ambient.blue = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "Kd") && matOpen) {
			//Diffuse color
			currMat->diffuse.red = atof(strtok(NULL, " \t"));
			currMat->diffuse.green = atof(strtok(NULL, " \t"));
			currMat->diffuse.blue = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "Ks") && matOpen) {
			//Specular color
			currMat->specular.red = atof(strtok(NULL, " \t"));
			currMat->specular.green = atof(strtok(NULL, " \t"));
			currMat->specular.blue = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "Ke") && matOpen) {
			//Emissive color
			currMat->emission.red = atof(strtok(NULL, " \t"));
			currMat->emission.green = atof(strtok(NULL, " \t"));
			currMat->emission.blue = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "Ns") && matOpen) {
			//Shinyness
			//UNUSED
		} else if (stringEquals(token, "d") && matOpen) {
			//Transparency
			currMat->transparency = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "r") && matOpen) {
			//Reflectivity
			currMat->reflectivity = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "sharpness") && matOpen) {
			//Glossiness
			currMat->glossiness = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "Ni") && matOpen) {
			//IOR
			currMat->IOR = atof(strtok(NULL, " \t"));
		} else if (stringEquals(token, "illum") && matOpen) {
			//Illumination type
			//UNUSED
		} else if (stringEquals(token, "map_Kd") && matOpen) {
			//Diffuse texture map
			strncpy(currMat->textureFilePath, strtok(NULL, " \t"), CRAY_MESH_FILENAME_LENGTH);
		} else if ((stringEquals(token, "map_bump") || stringEquals(token, "bump")) && matOpen) {
			//Bump map
			//TODO
		} else if (stringEquals(token, "map_d") && matOpen) {
			//Alpha channel? Not needed I think
		} else {
			logr(warning, "Unrecognized command '%s' in mtl file %s on line %i\n", token, filePath, linenum);
		}
	}
	
	fclose(fileStream);
	
	*mtlCount = count;
	
	for (int i = 0; i < *mtlCount; i++) {
		newMaterials[i].name[strcspn(newMaterials[i].name, "\n")] = 0;
	}
	
	return newMaterials;
}
