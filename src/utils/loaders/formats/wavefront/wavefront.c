//
//  wavefront.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
//

#include <string.h>
#include "../../../../includes.h"
#include "../../../../datatypes/mesh.h"
#include "../../../../datatypes/vector.h"
#include "../../../../datatypes/poly.h"
#include "../../../../datatypes/material.h"
#include "../../../logging.h"
#include "../../../string.h"
#include "../../../fileio.h"
#include "../../../textbuffer.h"
#include "mtlloader.h"

#include "wavefront.h"

static struct vector parseVertex(lineBuffer *line) {
	ASSERT(line->amountOf.tokens == 4);
	return (struct vector){ atof(nextToken(line)), atof(nextToken(line)), atof(nextToken(line)) };
}

static struct coord parseCoord(lineBuffer *line) {
	// Some weird OBJ files just have a 0.0 as the third value for 2d coordinates.
	ASSERT(line->amountOf.tokens == 3 || line->amountOf.tokens == 4);
	return (struct coord){ atof(nextToken(line)), atof(nextToken(line)) };
}

// Wavefront supports different indexing types like
// f v1 v2 v3 [v4]
// f v1/vt1 v2/vt2 v3/vt3
// f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
// f v1//vn1 v2//vn2 v3//vn3
// Or a quad:
// f v1//vn1 v2//vn2 v3//vn3 v4//vn4
static inline size_t parse_polys(lineBuffer *line, struct poly *buf) {
	char container[LINEBUFFER_MAXSIZE];
	lineBuffer batch = { .buf = container };
	size_t polycount = line->amountOf.tokens - 3;
	// For now, c-ray will just translate quads to two polygons while parsing
	// Explode in a ball of fire if we encounter an ngon
	bool is_ngon = polycount > 2;
	if (is_ngon) {
		logr(debug, "!! Found an ngon in wavefront file, skipping !!\n");
		polycount = 2;
	}
	bool skipped = false;
	for (size_t i = 0; i < polycount; ++i) {
		firstToken(line);
		struct poly *p = &buf[i];
		p->vertexCount = MAX_CRAY_VERTEX_COUNT;
		for (int j = 0; j < p->vertexCount; ++j) {
			fillLineBuffer(&batch, nextToken(line), '/');
			if (batch.amountOf.tokens >= 1) p->vertexIndex[j] = atoi(firstToken(&batch));
			if (batch.amountOf.tokens >= 2) p->textureIndex[j] = atoi(nextToken(&batch));
			if (batch.amountOf.tokens >= 3) p->normalIndex[j] = atoi(nextToken(&batch));
			if (i == 1 && !skipped) {
				nextToken(line);
				skipped = true;
			}
		}
	}
	return polycount;
}

static int fixIndex(size_t max, int oldIndex) {
	if (oldIndex == 0) // Unused
		return -1;
	
	if (oldIndex < 0) // Relative to end of list
		return (int)max + oldIndex;
	
	return oldIndex - 1;// Normal indexing
}

static void fixIndices(struct poly *p, size_t totalVertices, size_t totalTexCoords, size_t totalNormals) {
	for (int i = 0; i < MAX_CRAY_VERTEX_COUNT; ++i) {
		p->vertexIndex[i] = fixIndex(totalVertices, p->vertexIndex[i]);
		p->textureIndex[i] = fixIndex(totalTexCoords, p->textureIndex[i]);
		p->normalIndex[i] = fixIndex(totalNormals, p->normalIndex[i]);
	}
}

float get_poly_area(struct poly *p, struct vector *vertices) {
	const struct vector v0 = vertices[p->vertexIndex[0]];
	const struct vector v1 = vertices[p->vertexIndex[1]];
	const struct vector v2 = vertices[p->vertexIndex[2]];

	const struct vector a = vec_sub(v1, v0);
	const struct vector b = vec_sub(v2, v0);

	const struct vector cross = vec_cross(a, b);
	return vec_length(cross) / 2.0f;
}

struct mesh_arr parse_wavefront(const char *file_path, struct file_cache *cache) {
	size_t bytes = 0;
	char *input = load_file(file_path, &bytes, cache);
	if (!input) return (struct mesh_arr){ 0 };
	logr(debug, "Loading OBJ %s\n", file_path);
	textBuffer *file = newTextBuffer(input);
	char *assetPath = get_file_path(file_path);
	
	char buf[LINEBUFFER_MAXSIZE];
	// Count totals and preallocate space
	size_t total_vertices  = 0;
	size_t total_texcoords = 0;
	size_t total_normals   = 0;
	size_t total_polys     = 0;
	char *head = firstLine(file);
	lineBuffer line0 = { .buf = buf };
	while (head) {
		if (head[0] == 'v') {
			total_vertices++;
		} else if (head[0] == 'f') {
			fillLineBuffer(&line0, head, ' ');
			if (line0.amountOf.tokens > 4)
				total_polys += 2;
			else
				total_polys++;
		} else if (head[0] == 'v' && head[1] == 'n') {
			total_normals++;
		} else if (head[0] == 'v' && head[1] == 't') {
			total_texcoords++;
		}
		head = nextLine(file);
	};

	int current_material_idx = 0;
	
	struct mesh_arr meshes = { 0 };
	struct mesh *current = NULL;
	
	struct vertex_buffer *file_buf = vertex_buf_ref(NULL);
	struct material_buffer *mtllib = material_buf_ref(NULL);
	
	struct poly polybuf[2];

	//Start processing line-by-line, state machine style.
	head = firstLine(file);
	lineBuffer line = { .buf = buf };
	while (head) {
		fillLineBuffer(&line, head, ' ');
		char *first = firstToken(&line);
		if (first[0] == '#') {
			head = nextLine(file);
			continue;
		} else if (first[0] == '\0') {
			head = nextLine(file);
			continue;
		} else if (first[0] == 'o'/* || first[0] == 'g'*/) { //FIXME: o and g probably have a distinction for a reason?
			size_t idx = mesh_arr_add(&meshes, (struct mesh){ 0 });
			current = &meshes.items[idx];
			current->name = stringCopy(peekNextToken(&line));
		} else if (stringEquals(first, "v")) {
			vector_arr_add(&file_buf->vertices, parseVertex(&line));
		} else if (stringEquals(first, "vt")) {
			coord_arr_add(&file_buf->texture_coords, parseCoord(&line));
		} else if (stringEquals(first, "vn")) {
			vector_arr_add(&file_buf->normals, parseVertex(&line));
		} else if (stringEquals(first, "s")) {
			// Smoothing groups. We don't care about these, we always smooth.
		} else if (stringEquals(first, "f")) {
			size_t count = parse_polys(&line, polybuf);
			for (size_t i = 0; i < count; ++i) {
				struct poly p = polybuf[i];
				//TODO: Check if we actually need the file totals here
				// fixIndices(&p, current->vertices.count, current->texture_coords.count, current->normals.count);
				fixIndices(&p, total_vertices, total_texcoords, total_normals);
				//FIXME
				// current->surface_area += get_poly_area(&p, current->vertices.items);
				p.materialIndex = current_material_idx;
				p.hasNormals = p.normalIndex[0] != -1;
				poly_arr_add(&current->polygons, p);
			}
		} else if (stringEquals(first, "usemtl")) {
			char *name = peekNextToken(&line);
			current_material_idx = 0;
			for (size_t i = 0; i < mtllib->materials.count; ++i) {
				if (stringEquals(mtllib->materials.items[i].name, name)) {
					current_material_idx = i;
				}
			}
		} else if (stringEquals(first, "mtllib")) {
			char *mtlFilePath = stringConcat(assetPath, peekNextToken(&line));
			windowsFixPath(mtlFilePath);
			//FIXME: Handle multiple mtllibs
			mtllib->materials = parse_mtllib(mtlFilePath, cache);
			free(mtlFilePath);
		} else {
			char *fileName = get_file_name(file_path);
			logr(debug, "Unknown statement \"%s\" in OBJ \"%s\" on line %zu\n",
				 first, fileName, file->current.line);
			free(fileName);
		}
		head = nextLine(file);
	}
	
	destroyTextBuffer(file);
	free(input);
	free(assetPath);

	if (!mtllib->materials.count) {
		material_arr_add(&mtllib->materials, warningMaterial());
	}

	for (size_t i = 0; i < meshes.count; ++i) {
		meshes.items[i].mbuf = material_buf_ref(mtllib);
	}

	for (size_t i = 0; i < meshes.count; ++i) {
		struct mesh *m = &meshes.items[i];
		m->vbuf = vertex_buf_ref(file_buf);
		logr(debug, "Mesh %s surface area is %.4fm²\n", m->name, (double)m->surface_area);
	}

	vertex_buf_unref(file_buf);
	material_buf_unref(mtllib);
	
	return meshes;
}
