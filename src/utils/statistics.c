//
//  statistics.c
//  C-ray
//
//  Created by Valtteri on 18.2.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "statistics.h"

#include "../utils/logging.h"
#include "../utils/assert.h"
#include <string.h>
#include <stdlib.h>

struct stats {
	bool enabled;
	//Memory
	unsigned long kd_tree_bytes;
	unsigned long image_buffer_bytes;
	unsigned long mesh_bytes;
	
	unsigned long raw_bytes_allocated;
	unsigned long raw_bytes_freed;
	
	//Scene
	unsigned long lights;
	unsigned long materials;
	unsigned long meshes;
	unsigned long spheres;
	
	//Counters
	unsigned long paths;
	unsigned long path_lengths; //div by paths to get avg path length
	
	unsigned long calls_to_allocate;
	unsigned long calls_to_free;
};

void clear_stats(struct stats *s) {
	memset(s, 0, sizeof(*s));
}

void toggle_stats(struct stats *s) {
	s->enabled = !s->enabled;
}

bool stats_enabled(struct stats *s) {
	return s->enabled;
}

void increment(struct stats *s, enum counter c, unsigned long amount) {
	if (!s->enabled) return;
	
	switch (c) {
		case kd_tree_bytes:
			s->kd_tree_bytes += amount;
			break;
		case image_buffer_bytes:
			s->image_buffer_bytes += amount;
			break;
		case mesh_bytes:
			s->mesh_bytes += amount;
			break;
		case raw_bytes_allocated:
			s->raw_bytes_allocated += amount;
			break;
		case raw_bytes_freed:
			s->raw_bytes_freed += amount;
			break;
		case lights:
			s->lights += amount;
			break;
		case materials:
			s->materials += amount;
			break;
		case meshes:
			s->meshes += amount;
			break;
		case spheres:
			s->spheres += amount;
			break;
		case paths:
			s->paths += amount;
			break;
		case path_lengths:
			s->path_lengths += amount;
			break;
		case calls_to_allocate:
			s->calls_to_allocate += amount;
			break;
		case calls_to_free:
			s->calls_to_free += amount;
			break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

unsigned long get_value(struct stats *s, enum counter c) {
	switch (c) {
		case kd_tree_bytes:
			return s->kd_tree_bytes;
		case image_buffer_bytes:
			return s->image_buffer_bytes;
		case mesh_bytes:
			return s->mesh_bytes;
		case raw_bytes_allocated:
			return s->raw_bytes_allocated;
		case raw_bytes_freed:
			return s->raw_bytes_freed;
		case lights:
			return s->lights;
		case materials:
			return s->materials;
		case meshes:
			return s->meshes;
		case spheres:
			return s->spheres;
		case paths:
			return s->paths;
		case path_lengths:
			return s->path_lengths;
		case avg_path_length:
			return 0;
		case calls_to_allocate:
			return s->calls_to_allocate;
		case calls_to_free:
			return s->calls_to_free;
		default:
			ASSERT_NOT_REACHED();
			return 0;
	}
}

void print_stats() {
	logr(debug, "TODO");
}
