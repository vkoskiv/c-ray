//
//  statistics.h
//  C-ray
//
//  Created by Valtteri on 18.2.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

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
	
	calls_to_allocate,
	calls_to_free
};

struct stats;

void clear_stats(void);

struct stats *copy_stats(void);

/// Toggle statistics gathering on or off
void toggle_stats(void);

void increment(enum counter c, unsigned long amount);

void print_stats(void);
