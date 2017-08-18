//
//  camera.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 02/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "camera.h"

#include "filehandler.h"
#include "scene.h"

void computeFocalLength(struct scene *scene) {
	//Focal length is calculated based on the camera FOV value
	if (scene->camera->FOV > 0.0 && scene->camera->FOV < 189.0) {
		scene->camera->focalLength = 0.5 * scene->image->size.width / tanf((double)(PIOVER180) * 0.5 * scene->camera->FOV);
	}
}
