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
#include <stdbool.h>

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

//Global statistics here
static struct stats g_stats = {0};

void clear_stats() {
	memset(&g_stats, 0, sizeof(g_stats));
}

struct stats *copy_stats() {
	struct stats *copy = malloc(sizeof(copy));
	*copy = g_stats;
	return copy;
}

void toggle_stats() {
	g_stats.enabled = !g_stats.enabled;
}

void increment(enum counter c, unsigned long amount) {
	if (!g_stats.enabled) return;
	
	switch (c) {
		case kd_tree_bytes:
			g_stats.kd_tree_bytes += amount;
			break;
		case image_buffer_bytes:
			g_stats.image_buffer_bytes += amount;
			break;
		case mesh_bytes:
			g_stats.mesh_bytes += amount;
			break;
		case raw_bytes_allocated:
			g_stats.raw_bytes_allocated += amount;
			break;
		case raw_bytes_freed:
			g_stats.raw_bytes_freed += amount;
			break;
		case lights:
			g_stats.lights += amount;
			break;
		case materials:
			g_stats.materials += amount;
			break;
		case meshes:
			g_stats.meshes += amount;
			break;
		case spheres:
			g_stats.spheres += amount;
			break;
		case paths:
			g_stats.paths += amount;
			break;
		case path_lengths:
			g_stats.path_lengths += amount;
			break;
		case calls_to_allocate:
			g_stats.calls_to_allocate += amount;
			break;
		case calls_to_free:
			g_stats.calls_to_free += amount;
			break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

unsigned long get_value(enum counter c) {
	switch (c) {
		case kd_tree_bytes:
			return g_stats.kd_tree_bytes;
			break;
		case image_buffer_bytes:
			return g_stats.image_buffer_bytes;
			break;
		case mesh_bytes:
			return g_stats.mesh_bytes;
			break;
		case raw_bytes_allocated:
			return g_stats.raw_bytes_allocated;
			break;
		case raw_bytes_freed:
			return g_stats.raw_bytes_freed;
			break;
		case lights:
			return g_stats.lights;
			break;
		case materials:
			return g_stats.materials;
			break;
		case meshes:
			return g_stats.meshes;
			break;
		case spheres:
			return g_stats.spheres;
			break;
		case paths:
			return g_stats.paths;
			break;
		case path_lengths:
			return g_stats.path_lengths;
			break;
		case calls_to_allocate:
			return g_stats.calls_to_allocate;
			break;
		case calls_to_free:
			return g_stats.calls_to_free;
			break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

void print_stats() {
	logr(debug, "TODO");
}
