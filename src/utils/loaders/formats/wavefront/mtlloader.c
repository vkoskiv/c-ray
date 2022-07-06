//
//  mtlloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

// C-ray MTL file parser

#include "../../../../includes.h"
#include "mtlloader.h"

#include "../../../../datatypes/material.h"
#include "../../../logging.h"
#include "../../../string.h"
#include "../../../textbuffer.h"
#include "../../../fileio.h"
#include "../../../assert.h"
#include "../../textureloader.h"

static size_t countMaterials(textBuffer *buffer) {
	size_t mtlCount = 0;
	char *head = firstLine(buffer);
	while (head) {
		if (stringStartsWith("newmtl", head)) mtlCount++;
		head = nextLine(buffer);
	}
	logr(debug, "File contains %zu materials\n", mtlCount);
	firstLine(buffer);
	return mtlCount;
}

static struct color parse_color(lineBuffer *line) {
	ASSERT(line->amountOf.tokens == 4);
	return (struct color){ atof(nextToken(line)), atof(nextToken(line)), atof(nextToken(line)), 1.0f };
}

struct material *parseMTLFile(const char *filePath, int *mtlCount, struct file_cache *cache) {
	size_t bytes = 0;
	char *rawText = loadFile(filePath, &bytes, cache);
	if (!rawText) return NULL;
	logr(debug, "Loading MTL at %s\n", filePath);
	textBuffer *file = newTextBuffer(rawText);
	free(rawText);
	
	char *assetPath = getFilePath(filePath);
	
	size_t materialAmount = countMaterials(file);
	struct material *materials = calloc(materialAmount, sizeof(*materials));
	size_t currentMaterialIdx = 0;
	struct material *current = NULL;
	
	char *head = firstLine(file);
	lineBuffer *line = newLineBuffer();
	while (head) {
		fillLineBuffer(line, head, ' ');
		char *first = firstToken(line);
		if (first[0] == '#') {
			head = nextLine(file);
			continue;
		} else if (head[0] == '\0') {
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
			current->ambient = parse_color(line);
		} else if (stringEquals(first, "Kd")) {
			current->diffuse = parse_color(line);
		} else if (stringEquals(first, "Ks")) {
			current->specular = parse_color(line);
		} else if (stringEquals(first, "Ke")) {
			current->emission = parse_color(line);
		} else if (stringEquals(first, "illum")) {
			current->illum = atoi(nextToken(line));
		} else if (stringEquals(first, "Ns")) {
			current->shinyness = atof(nextToken(line));
		} else if (stringEquals(first, "d")) {
			current->transparency = atof(nextToken(line));
		} else if (stringEquals(first, "r")) {
			current->reflectivity = atof(nextToken(line));
		} else if (stringEquals(first, "sharpness")) {
			current->glossiness = atof(nextToken(line));
		} else if (stringEquals(first, "Ni")) {
			current->IOR = atof(nextToken(line));
		} else if (stringEquals(first, "map_Kd") || stringEquals(first, "map_Ka")) {
			char *path = stringConcat(assetPath, nextToken(line));
			windowsFixPath(path);
			current->texture = load_texture(path, NULL, cache);
			free(path);
		} else if (stringEquals(first, "norm") || stringEquals(first, "bump") || stringEquals(first, "map_bump")) {
			char *path = stringConcat(assetPath, nextToken(line));
			windowsFixPath(path);
			current->normalMap = load_texture(path, NULL, cache);
			free(path);
		} else if (stringEquals(first, "map_Ns")) {
			char *path = stringConcat(assetPath, nextToken(line));
			windowsFixPath(path);
			current->specularMap = load_texture(path, NULL, cache);
			free(path);
		} else {
			char *fileName = getFileName(filePath);
			logr(debug, "Unknown statement \"%s\" in MTL \"%s\" on line %zu\n",
				first, fileName, file->current.line);
			free(fileName);
		}
		head = nextLine(file);
	}
	
	destroyLineBuffer(line);
	destroyTextBuffer(file);
	free(assetPath);
	if (mtlCount) *mtlCount = (int)materialAmount;
	return materials;
}
