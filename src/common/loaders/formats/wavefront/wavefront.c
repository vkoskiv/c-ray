//
//  wavefront.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdint.h>
#include <string.h>
#include "../../../../includes.h"
#include <vector.h>
#include <logging.h>
#include <cr_string.h>
#include <fileio.h>
#include <textbuffer.h>
#include <loaders/meshloader.h>
#include <c-ray/c-ray.h>
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
static inline size_t parse_polys(lineBuffer *line, struct cr_face *buf) {
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
		struct cr_face *p = &buf[i];
		for (int j = 0; j < MAX_CRAY_VERTEX_COUNT; ++j) {
			fillLineBuffer(&batch, nextToken(line), '/');
			if (batch.amountOf.tokens >= 1) p->vertex_idx[j] = atoi(firstToken(&batch));
			if (batch.amountOf.tokens >= 2) p->texture_idx[j] = atoi(nextToken(&batch));
			if (batch.amountOf.tokens >= 3) p->normal_idx[j] = atoi(nextToken(&batch));
			if (i == 1 && !skipped) {
				nextToken(line);
				skipped = true;
			}
		}
	}
	return polycount;
}

static inline int fixIndex(int idx) {
	if (idx == 0) return -1; // Unused
	// Wavefront is supposed to support negative indexing, which
	// would mean we would index off the end of the vertex list.
	// But supporting that makes parsing super inefficient, and
	// I've never seen a file that actually does that, so we don't
	// support it anymore.
	return idx - 1;// Normal indexing
}

static inline void fixIndices(struct cr_face *p) {
	for (int i = 0; i < MAX_CRAY_VERTEX_COUNT; ++i) {
		p->vertex_idx[i] = fixIndex(p->vertex_idx[i]);
		p->texture_idx[i] = fixIndex(p->texture_idx[i]);
		p->normal_idx[i] = fixIndex(p->normal_idx[i]);
	}
}

float get_poly_area(struct cr_face *p, struct vector *vertices) {
	const struct vector v0 = vertices[p->vertex_idx[0]];
	const struct vector v1 = vertices[p->vertex_idx[1]];
	const struct vector v2 = vertices[p->vertex_idx[2]];

	const struct vector a = vec_sub(v1, v0);
	const struct vector b = vec_sub(v2, v0);

	const struct vector cross = vec_cross(a, b);
	return vec_length(cross) / 2.0f;
}

struct mesh_parse_result parse_wavefront(const char *file_path) {
	file_data input = file_load(file_path);
	if (!input)
		return (struct mesh_parse_result){ 0 };
	logr(debug, "Loading OBJ %s\n", file_path);
	textBuffer *file = newTextBuffer((char *)input, v_arr_len(input));
	char *assetPath = get_file_path(file_path);
	
	char buf[LINEBUFFER_MAXSIZE];

	int current_material_idx = 0;
	
	struct ext_mesh *current = NULL;
	
	struct mesh_parse_result result = { 0 };
	
	struct cr_face polybuf[2];

	//Start processing line-by-line, state machine style.
	char *head = firstLine(file);
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
			size_t idx = v_arr_add(result.meshes, (struct ext_mesh){ 0 });
			current = &result.meshes[idx];
			current->name = stringCopy(peekNextToken(&line));
		} else if (stringEquals(first, "v")) {
			v_arr_add(result.geometry.vertices, parseVertex(&line));
		} else if (stringEquals(first, "vt")) {
			v_arr_add(result.geometry.texture_coords, parseCoord(&line));
		} else if (stringEquals(first, "vn")) {
			v_arr_add(result.geometry.normals, parseVertex(&line));
		} else if (stringEquals(first, "s")) {
			// Smoothing groups. We don't care about these, we always smooth.
		} else if (stringEquals(first, "f")) {
			size_t count = parse_polys(&line, polybuf);
			for (size_t i = 0; i < count; ++i) {
				struct cr_face p = polybuf[i];
				fixIndices(&p);
				//FIXME
				// current->surface_area += get_poly_area(&p, current->vertices);
				p.mat_idx = current_material_idx;
				p.has_normals= p.normal_idx[0] != -1;
				v_arr_add(current->faces, p);
			}
		} else if (stringEquals(first, "usemtl")) {
			char *name = peekNextToken(&line);
			current_material_idx = 0;
			for (size_t i = 0; i < v_arr_len(result.materials); ++i) {
				if (stringEquals(result.materials[i].name, name)) {
					current_material_idx = i;
				}
			}
		} else if (stringEquals(first, "mtllib")) {
			char *mtlFilePath = stringConcat(assetPath, peekNextToken(&line));
			windowsFixPath(mtlFilePath);
			//FIXME: Handle multiple mtllibs
			ASSERT(!v_arr_len(result.materials));
			result.materials = parse_mtllib(mtlFilePath);
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
	v_arr_free(input);
	free(assetPath);

	if (!v_arr_len(result.materials)) {
		v_arr_add(result.materials, (struct mesh_material){
			.mat = NULL,
			.name = stringCopy("Unknown")
		});
	}

	return result;
}
