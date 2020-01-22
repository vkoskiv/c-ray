//
//  learn.h
//  C-Ray
//
//  Created by Valtteri on 16/10/2018.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../libraries/Tinn.h"

struct model {
	int trainIterations;
	int sessions;
	
	int nips;
	int nops;
	int nhid;
	//Single model, but for each training iteration we just train with the 3 color channels separately
	char *modelPath;
	Tinn network;
};

struct data {
	//3D matrices. 3x 2D tile.
	float ***source;
	float ***target;
	
	int nips;
	int nops;
};

void study(void);
