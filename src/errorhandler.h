//
//  errorhandler.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 14/09/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum renderLog {
	threadMallocFailed,
	sceneBuildFailed,
	imageMallocFailed,
	invalidThreadCount,
	threadFrozen,
	threadCreateFailed,
	threadRemoveFailed,
	sceneDebugEnabled,
	sceneParseErrorScene,
	sceneParseErrorCamera,
	sceneParseErrorSphere,
	sceneParseErrorPoly,
	sceneParseErrorLight,
	sceneParseErrorMaterial,
	sceneParseErrorMalloc,
	sceneParseErrorNoPath,
	dontTurnOnTheAntialiasingYouDoofus,
	renderErrorInvalidSampleCount,
	drawTaskMallocFailed,
	defaultError
};

enum logSource {
	rendererSource,
	sceneBuilder,
	vectorHandler,
	colorHandler,
	polyHandler,
	sphereHandler,
	fileHandler,
	defaultSource
};

void logHandler(enum renderLog error);
