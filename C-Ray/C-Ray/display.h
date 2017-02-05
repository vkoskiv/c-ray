//
//  display.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 31/12/2016.
//  Copyright © 2016 Valtteri Koskivuori. All rights reserved.
//

#ifndef display_h
#define display_h

#include "includes.h"

typedef struct {
	int x, y;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	bool isDrawn;
}drawTask;

typedef struct {
	pthread_t thread_id;
	int thread_num;
}threadInfo;

void addDrawTask(threadInfo *tinfo, int x, int y, unsigned char red, unsigned char green, unsigned char blue);
void *drawThread(void *arg);
drawTask getTask();
bool drawTasksEmpty();
void addTask(drawTask data);



#endif /* display_h */
