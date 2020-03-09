//
//  statistics.h
//  C-ray
//
//  Created by Valtteri on 18.2.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>

enum counter {
	//Memory
	kd_tree_bytes,
	image_buffer_bytes,
	mesh_bytes,
	
	raw_bytes_allocated,
	raw_bytes_freed,
	
	//Scene
	lights,
	materials,
	meshes,
	spheres,
	
	//Counters
	paths,
	path_lengths,//div by paths to get avg path length
	avg_path_length,
	
	calls_to_allocate,
	calls_to_free
};

struct stats;

void clear_stats(struct stats *s);

/// Toggle statistics gathering on or off
void toggle_stats(struct stats *s);

bool stats_enabled(struct stats *s);

void increment(struct stats *s, enum counter c, unsigned long amount);
unsigned long get_value(struct stats *s, enum counter c);

void print_stats(void);
