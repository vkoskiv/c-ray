//
//  c-ray.c
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "c-ray.h"

#include "renderer/renderer.h"
#include "datatypes/scene.h"
#include "utils/gitsha1.h"
#include "utils/logging.h"
#include "utils/filehandler.h"
#include "utils/multiplatform.h"
#include "utils/assert.h"
#include "datatypes/texture.h"
#include "utils/ui.h"
#include "utils/timer.h"

#define VERSION "0.6.3"

//Internal renderer state
struct renderer *grenderer = NULL;

struct texture *currentImage = NULL;

char *crGetVersion() {
	return VERSION;
}

char *crGitHash(int chars) {
	return gitHash(chars);
}

void crInitTerminal() {
	initTerminal();
}

void crInitSDL() {
#ifdef UI_ENABLED
	ASSERT(!grenderer->mainDisplay->window);
	initSDL(grenderer->mainDisplay);
#endif
}

void crDestroySDL() {
#ifdef UI_ENABLED
	ASSERT(grenderer->mainDisplay->window);
	freeDisplay(grenderer->mainDisplay);
#endif
}

void crWriteImage() {
	char *hash = gitHash(8);
	char buf[64];
	smartTime(getMs(*grenderer->state.timer), buf);
	if (currentImage) {
		if (!grenderer->state.renderAborted) {
			writeImage(currentImage, (struct renderInfo){
				.bounces = crGetBounces(),
				.samples = crGetSampleCount(),
				.crayVersion = crGetVersion(),
				.gitHash = hash,
				.renderTime = buf,
				.threadCount = crGetThreadCount()
			});
		} else {
			logr(info, "Abort pressed, image won't be saved.\n");
		}
	}
	free(hash);
}

char *crLoadFile(char *filePath, size_t *bytes) {
	return loadFile(filePath, bytes);
}

char *crReadStdin(size_t *bytes) {
	return readStdin(bytes);
}

void crInitRenderer() {
	ASSERT(!grenderer);
	grenderer = newRenderer();
}

void crDestroyRenderer() {
	ASSERT(grenderer);
	freeRenderer(grenderer);
	if (currentImage) {
		freeTexture(currentImage);
		free(currentImage);
	}
}

int crLoadSceneFromFile(char *filePath) {
	size_t bytes = 0;
	char *input = loadFile(filePath, &bytes);
	if (input) {
		if (loadScene(grenderer, filePath) != 0) {
			free(input);
			return -1;
		}
	} else {
		return -1;
	}
	free(input);
	return 0;
}

int crLoadSceneFromBuf(char *buf) {
	return loadScene(grenderer, buf);
}

void crSetRenderOrder(void) {
	ASSERT_NOT_REACHED();
}

void crGetRenderOrder(void) {
	ASSERT_NOT_REACHED();
}

void crSetThreadCount(int threadCount, bool fromSystem) {
	grenderer->prefs.threadCount = threadCount;
	grenderer->prefs.fromSystem = fromSystem;
	crRestartInteractive();
}

int crGetThreadCount(void) {
	return grenderer->prefs.threadCount;
}

void crSetSampleCount(int sampleCount) {
	(void)sampleCount;
	ASSERT_NOT_REACHED();
}

int crGetSampleCount(void) {
	return grenderer->prefs.sampleCount;
}

void crSetBounces(int bounces) {
	(void)bounces;
	ASSERT_NOT_REACHED();
}

int crGetBounces(void) {
	return grenderer->prefs.bounces;
}

void crSetTileWidth(int width) {
	(void)width;
	ASSERT_NOT_REACHED();
}

int crGetTileWidth(void) {
	return grenderer->prefs.tileWidth;
}

void crSetTileHeight(int height) {
	(void)height;
	ASSERT_NOT_REACHED();
}

int crGetTileHeight(void) {
	return grenderer->prefs.tileHeight;
}

void crSetImageWidth(int width) {
	grenderer->prefs.imageWidth = width;
	crRestartInteractive();
}

int crGetImageWidth(void) {
	return grenderer->prefs.imageWidth;
}

void crSetImageHeight(int height) {
	(void)height;
	ASSERT_NOT_REACHED();
}

int crGetImageHeight() {
	return grenderer->prefs.imageHeight;
}

void crSetFilePath(char *filePath) {
	(void)filePath;
	ASSERT_NOT_REACHED();
}

char *crGetFilePath() {
	return grenderer->prefs.imgFilePath;
}

void crSetFileName(char *fileName) {
	(void)fileName;
	ASSERT_NOT_REACHED();
}

char *crGetFileName() {
	return grenderer->prefs.imgFileName;
}

void crSetAntialiasing(bool on) {
	grenderer->prefs.antialiasing = on;
}

bool crGetAntialiasing() {
	return grenderer->prefs.antialiasing;
}

void crRenderSingleFrame() {
	startTimer(grenderer->state.timer);
	currentImage = renderFrame(grenderer);
	printDuration(getMs(*grenderer->state.timer));
}

//Interactive mode
void crStartInteractive(void) {
	ASSERT_NOT_REACHED();
}

//Toggle paused state
void crPauseInteractive(void) {
	ASSERT_NOT_REACHED();
}

//Just get the current buffer
void crGetCurrentImage(void) {
	ASSERT_NOT_REACHED();
}
void crRestartInteractive() {
	//if (grenderer->prefs.interactive) { do the thing }
	ASSERT_NOT_REACHED();
}

void crTransformMesh(void); //Transform, recompute kd-tree, restart

void crMoveCamera(void/*struct dimension delta*/);
void crSetHDR(void);
