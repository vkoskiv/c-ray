//
//  learn.h
//  C-Ray
//
//  Created by Valtteri on 16/10/2018.
//  Copyright © 2018 Valtteri Koskivuori. All rights reserved.
//

#pragma once

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

struct trainingTile {
	float *vals;
};

void study(void);
