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

static struct color parse_color(lineBuffer *line) {
	ASSERT(line->amountOf.tokens == 4);
	return (struct color){ atof(nextToken(line)), atof(nextToken(line)), atof(nextToken(line)), 1.0f };
}

struct material_arr parse_mtllib(const char *filePath, struct file_cache *cache) {
	file_data mtllib_text = file_load(filePath, cache);
	if (!mtllib_text.count) return (struct material_arr){ 0 };
	logr(debug, "Loading MTL at %s\n", filePath);
	textBuffer *file = newTextBuffer((char *)mtllib_text.items);
	file_free(&mtllib_text);
	
	char *assetPath = get_file_path(filePath);
	
	struct material_arr materials = { 0 };
	struct material *current = NULL;
	
	char *head = firstLine(file);
	char buf[LINEBUFFER_MAXSIZE];
	lineBuffer line = { .buf = buf };
	while (head) {
		fillLineBuffer(&line, head, ' ');
		char *first = firstToken(&line);
		if (first[0] == '#') {
			head = nextLine(file);
			continue;
		} else if (head[0] == '\0') {
			head = nextLine(file);
			continue;
		} else if (stringEquals(first, "newmtl")) {
			size_t idx = material_arr_add(&materials, (struct material){ 0 });
			current = &materials.items[idx];
			if (!peekNextToken(&line)) {
				logr(warning, "newmtl without a name on line %zu\n", line.current.line);
				material_arr_free(&materials);
				return (struct material_arr){ 0 };
			}
			current->name = stringCopy(peekNextToken(&line));
		} else if (stringEquals(first, "Ka")) {
			// Ignore
		} else if (stringEquals(first, "Kd")) {
			current->diffuse = parse_color(&line);
		} else if (stringEquals(first, "Ks")) {
			current->specular = parse_color(&line);
		} else if (stringEquals(first, "Ke")) {
			current->emission = parse_color(&line);
		} else if (stringEquals(first, "illum")) {
			current->illum = atoi(nextToken(&line));
		} else if (stringEquals(first, "Ns")) {
			current->shinyness = atof(nextToken(&line));
		} else if (stringEquals(first, "d")) {
			current->transparency = atof(nextToken(&line));
		} else if (stringEquals(first, "r")) {
			current->reflectivity = atof(nextToken(&line));
		} else if (stringEquals(first, "sharpness")) {
			current->glossiness = atof(nextToken(&line));
		} else if (stringEquals(first, "Ni")) {
			current->IOR = atof(nextToken(&line));
		} else if (stringEquals(first, "map_Kd") || stringEquals(first, "map_Ka")) {
			char *path = stringConcat(assetPath, nextToken(&line));
			windowsFixPath(path);
			current->texture = load_texture(path, NULL, cache);
			free(path);
		} else if (stringEquals(first, "norm") || stringEquals(first, "bump") || stringEquals(first, "map_bump")) {
			char *path = stringConcat(assetPath, nextToken(&line));
			windowsFixPath(path);
			current->normalMap = load_texture(path, NULL, cache);
			free(path);
		} else if (stringEquals(first, "map_Ns")) {
			char *path = stringConcat(assetPath, nextToken(&line));
			windowsFixPath(path);
			current->specularMap = load_texture(path, NULL, cache);
			free(path);
		} else {
			char *fileName = get_file_name(filePath);
			logr(debug, "Unknown statement \"%s\" in MTL \"%s\" on line %zu\n",
				first, fileName, file->current.line);
			free(fileName);
		}
		head = nextLine(file);
	}
	
	destroyTextBuffer(file);
	free(assetPath);
	logr(debug, "Found %zu materials\n", materials.count);
	return materials;
}
