//
//  camera.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 02/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum fileType {
	bmp,
	png
};

enum renderOrder {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderToMiddle,
	renderOrderNormal
};

struct camera {
	int height, width; // Image dimensions
	enum fileType fileType;
	char *filePath;
	
	double FOV;
	double focalLength;
	double aperture;
	struct vector pos;
	
	unsigned char *imgData;
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
	
	bool newRenderer;
	
	bool useTiles;
	int tileWidth;
	int tileHeight;
};
