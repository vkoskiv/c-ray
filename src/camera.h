//
//  camera.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 02/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

struct camera {
	double FOV;
	double focalLength;
	double aperture;
	
	struct vector pos;
	struct vector up;
	struct vector left;
	
	int currentFrame;
	double contrast;
	double windowScale;
	
	bool isFullScreen;
	bool isBorderless;
	
	struct matrixTransform *transforms;
	int transformCount;
	
	bool areaLights;
	int bounces;
};

//Compute focal length for camera
void computeFocalLength(struct renderer *renderer);
void initCamera(struct camera *cam);
void transformCameraView(struct camera *cam, struct vector *direction); //For transforming direction in renderer
void transformCameraIntoView(struct camera *cam); //Run once in scene.c to calculate pos, up, left
