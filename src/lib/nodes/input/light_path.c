//
//  light_path.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 21/12/2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <datatypes/hitrecord.h>
#include <datatypes/scene.h>
#include <common/vector.h>
#include <common/vector.h>
#include <common/hashtable.h>
#include <nodes/bsdfnode.h>

#include "light_path.h"

struct light_path_node {
	struct valueNode node;
	enum cr_light_path_info query;
};

static bool compare(const void *A, const void *B) {
	const struct light_path_node *this = A;
	const struct light_path_node *other = B;
	return this->query == other->query;
}

static uint32_t hash(const void *p) {
	const struct light_path_node *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->node.eval, sizeof(this->node.eval));
	h = hashBytes(h, &this->query, sizeof(this->query));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	(void)node;
	snprintf(dumpbuf, bufsize, "light_path_node { }");
}

static float eval_is_camera(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->incident->type & rt_camera;
}

static float eval_is_shadow(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->incident->type & rt_shadow;
}

static float eval_is_diffuse(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->incident->type & rt_diffuse;
}

static float eval_is_glossy(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->incident->type & rt_glossy;
}

static float eval_is_singular(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->incident->type & rt_singular;
}

static float eval_is_reflection(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->incident->type & rt_reflection;
}

static float eval_is_transmission(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->incident->type & rt_transmission;
}

static float eval_ray_len(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record; (void)node; (void)sampler;
	return record->distance;
}

const struct valueNode *new_light_path(const struct node_storage *s, enum cr_light_path_info query) {
	float (*chosen_eval)(const struct valueNode *, sampler *, const struct hitRecord *) = NULL;
	switch (query) {
		case cr_is_camera_ray:
			chosen_eval = eval_is_camera;
			break;
		case cr_is_shadow_ray:
			chosen_eval = eval_is_shadow;
			break;
		case cr_is_diffuse_ray:
			chosen_eval = eval_is_diffuse;
			break;
		case cr_is_glossy_ray:
			chosen_eval = eval_is_glossy;
			break;
		case cr_is_singular_ray:
			chosen_eval = eval_is_singular;
			break;
		case cr_is_reflection_ray:
			chosen_eval = eval_is_reflection;
			break;
		case cr_is_transmission_ray:
			chosen_eval = eval_is_transmission;
			break;
		case cr_ray_length:
			chosen_eval = eval_ray_len;
			break;
	}
	if (!chosen_eval) {
		logr(warning, "Query %i not found, defaulting to ray_length\n", query);
		chosen_eval = eval_ray_len;
	}
	HASH_CONS(s->node_table, hash, struct light_path_node, {
		.node = {
			.eval = chosen_eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
