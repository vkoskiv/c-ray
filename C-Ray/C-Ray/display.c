//
//  display.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 31/12/2016.
//  Copyright © 2016 Valtteri Koskivuori. All rights reserved.
//

#include "display.h"

//UI Draw stuff

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void *drawThread(void *arg);
void addDrawTask(threadInfo *tinfo, int x, int y, unsigned char red, unsigned char green, unsigned char blue);
void renderUI(int x, int y, unsigned char red, unsigned char green, unsigned char blue);

#define UI_DRAW_BUFFERSIZE 32768

drawTask drawTaskQueue[UI_DRAW_BUFFERSIZE];
int front = 0;
int rear = -1;
int itemCount = 0;

drawTask peek() {
	return drawTaskQueue[front];
}

bool drawTasksEmpty() {
	return itemCount == 0;
}

bool drawTasksFull() {
	return itemCount == UI_DRAW_BUFFERSIZE;
}

int drawTaskAmount() {
	return itemCount;
}

void addTask(drawTask data) {
	pthread_mutex_lock(&mutex);
	if(!drawTasksFull()) {
		
		if(rear == UI_DRAW_BUFFERSIZE-1) {
			rear = -1;
		}
		
		drawTaskQueue[++rear] = data;
		itemCount++;
	}
	pthread_mutex_unlock(&mutex);
}

drawTask getTask() {
	pthread_mutex_lock(&mutex);
	drawTask task = drawTaskQueue[front++];
	
	if(front == UI_DRAW_BUFFERSIZE) {
		front = 0;
	}
	
	itemCount--;
	pthread_mutex_unlock(&mutex);
	return task;
}

void addDrawTask(threadInfo *tinfo, int x, int y, unsigned char red, unsigned char green, unsigned char blue) {
	if (!drawTasksFull()) {
		drawTask task;
		task.x = x;
		task.y = y;
		task.red = red;
		task.green = green;
		task.blue = blue;
		task.isDrawn = false;
		addTask(task);
	} else {
		printf("Missed draw task, (%i),(%i)\n", x, y);
	}
}
