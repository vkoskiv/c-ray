//
//  camera.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 02/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

enum renderOrder {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderToMiddle,
	renderOrderNormal
};

struct camera {
	double FOV;
	double focalLength;
	double aperture;
	struct vector pos;
	
	bool areaLights;
	bool antialiasing;
	int sampleCount;
	int threadCount;
	enum renderOrder tileOrder;
	int frameCount;
	int currentFrame;
	int bounces;
	double contrast;
	double windowScale;
	
	bool isFullScreen;
	bool isBorderless;
	
	struct matrixTransform *transforms;
	int transformCount;
	
	bool newRenderer;
	
	int tileWidth;
	int tileHeight;
};

//Compute focal length for camera
void computeFocalLength(struct renderer *renderer);
