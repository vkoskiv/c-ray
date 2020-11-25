//
//  mtlloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

// C-ray MTL file parser

#include "../../includes.h"
#include "mtlloader.h"

#include "../../datatypes/material.h"
#include "../../utils/logging.h"
#include "../../utils/string.h"
#include "../../utils/textbuffer.h"
#include "../../utils/fileio.h"
#include "../../utils/assert.h"
#include "../../utils/loaders/textureloader.h"

static size_t countMaterials(textBuffer *buffer) {
	size_t mtlCount = 0;
	char *head = firstLine(buffer);
	while (head) {
		if (stringStartsWith("newmtl", head)) mtlCount++;
		head = nextLine(buffer);
	}
	logr(debug, "File contains %zu materials\n", mtlCount);
	head = firstLine(buffer);
	return mtlCount;
}

struct color parseColor(lineBuffer *line) {
	ASSERT(line->amountOf.tokens == 4);
	return (struct color){atof(nextToken(line)), atof(nextToken(line)), atof(nextToken(line)), 1.0f};
}

struct material *parseMTLFile(char *filePath, int *mtlCount) {
	size_t bytes = 0;
	char *rawText = loadFile(filePath, &bytes);
	if (!rawText) return NULL;
	logr(debug, "Loading MTL at %s\n", filePath);
	textBuffer *file = newTextBuffer(rawText);
	
	char *assetPath = getFilePath(filePath);
	
	size_t materialAmount = countMaterials(file);
	struct material *materials = calloc(materialAmount, sizeof(*materials));
	size_t currentMaterialIdx = 0;
	struct material *current = NULL;
	
	char *head = firstLine(file);
	lineBuffer *line = newLineBuffer();
	while (head) {
		fillLineBuffer(line, head, " ");
		char *first = firstToken(line);
		if (first[0] == '#') {
			head = nextLine(file);
			continue;
		} else if (head[0] == '\0') {
			logr(debug, "empty line\n");
			head = nextLine(file);
			continue;
		} else if (stringEquals(first, "newmtl")) {
			current = &materials[currentMaterialIdx++];
			if (!peekNextToken(line)) {
				logr(warning, "newmtl without a name on line %zu\n", line->current.line);
				free(materials);
				return NULL;
			}
			current->name = stringCopy(peekNextToken(line));
		} else if (stringEquals(first, "Ka")) {
			current->ambient = parseColor(line);
		} else if (stringEquals(first, "Kd")) {
			current->diffuse = parseColor(line);
		} else if (stringEquals(first, "Ks")) {
			current->specular = parseColor(line);
		} else if (stringEquals(first, "Ke")) {
			current->emission = parseColor(line);
		} else if (stringEquals(first, "Ns")) {
			// Shinyness, unused
			head = nextLine(file);
			continue;
		} else if (stringEquals(first, "d")) {
			current->transparency = atof(nextToken(line));
		} else if (stringEquals(first, "r")) {
			current->reflectivity = atof(nextToken(line));
		} else if (stringEquals(first, "sharpness")) {
			current->glossiness = atof(nextToken(line));
		} else if (stringEquals(first, "Ni")) {
			current->IOR = atof(nextToken(line));
		} else if (stringEquals(first, "map_Kd")) {
			char *path = stringConcat(assetPath, nextToken(line));
			current->texture = loadTexture(path);
			free(path);
		} else if (stringEquals(first, "norm")) {
			char *path = stringConcat(assetPath, nextToken(line));
			current->normalMap = loadTexture(path);
			free(path);
		} else if (stringEquals(first, "map_Ns")) {
			char *path = stringConcat(assetPath, nextToken(line));
			current->specularMap = loadTexture(path);
			free(path);
		} else {
			char *fileName = getFileName(filePath);
			logr(warning, "Unknown statement \"%s\" in MTL \"%s\" on line %zu\n",
				first, fileName, file->current.line);
			free(fileName);
		}
		head = nextLine(file);
	}
	
	destroyLineBuffer(line);
	free(assetPath);
	if (mtlCount) *mtlCount = (int)materialAmount;
	return materials;
}

// Parse a list of materials and return an array of materials.
// mtlCount is the amount of materials loaded.
struct material *parseMTLFileOld(char *filePath, int *mtlCount) {
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
			currMat->name = calloc(CRAY_MATERIAL_NAME_SIZE, sizeof(*currMat->name));
			//currMat->textureFilePath = calloc(CRAY_MESH_FILENAME_LENGTH, sizeof(*currMat->textureFilePath));
			strncpy(currMat->name, strtok(NULL, " \t"), CRAY_MATERIAL_NAME_SIZE);
			matOpen = true;
		} else if (stringEquals(token, "Ka") && matOpen) {
			//Ambient color
			currMat->ambient.red = atof(strtok(NULL, " \t"));
			currMat->ambient.green = atof(strtok(NULL, " \t"));
			currMat->ambient.blue = atof(strtok(NULL, " \t"));
			currMat->ambient.alpha = 1.0f;
		} else if (stringEquals(token, "Kd") && matOpen) {
			//Diffuse color
			currMat->diffuse.red = atof(strtok(NULL, " \t"));
			currMat->diffuse.green = atof(strtok(NULL, " \t"));
			currMat->diffuse.blue = atof(strtok(NULL, " \t"));
			currMat->diffuse.alpha = 1.0f;
		} else if (stringEquals(token, "Ks") && matOpen) {
			//Specular color
			currMat->specular.red = atof(strtok(NULL, " \t"));
			currMat->specular.green = atof(strtok(NULL, " \t"));
			currMat->specular.blue = atof(strtok(NULL, " \t"));
			currMat->specular.alpha = 1.0f;
		} else if (stringEquals(token, "Ke") && matOpen) {
			//Emissive color
			currMat->emission.red = atof(strtok(NULL, " \t"));
			currMat->emission.green = atof(strtok(NULL, " \t"));
			currMat->emission.blue = atof(strtok(NULL, " \t"));
			currMat->emission.alpha = 1.0f;
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
			//strncpy(currMat->textureFilePath, strtok(NULL, " \t"), CRAY_MESH_FILENAME_LENGTH);
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
	
	if (mtlCount) *mtlCount = count;
	
	for (int i = 0; i < count; ++i) {
		newMaterials[i].name[strcspn(newMaterials[i].name, "\n")] = 0;
	}
	
	return newMaterials;
}
