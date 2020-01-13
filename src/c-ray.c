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

#define VERSION "0.6.2"

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
	smartTime(getMs(grenderer->state.timer), buf);
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

char *crReadStdin() {
	return readStdin();
}

void crInitRenderer() {
	grenderer = newRenderer();
}

//TODO: Remove
struct display *crGetDisplay() {
	return grenderer->mainDisplay;
}

void crDestroyRenderer() {
	freeRenderer(grenderer);
}

int crLoadSceneFromFile(char *filePath) {
	size_t bytes = 0;
	char *input = loadFile(filePath, &bytes);
	if (input) {
		if (loadScene(grenderer, filePath) != 0) {
			return -1;
		}
	} else {
		return -1;
	}
	return 0;
}

int crLoadSceneFromBuf(char *buf) {
	if (loadScene(grenderer, buf) != 0) {
		return -1;
	}
	return 0;
}

void crSetFileMode() {
	ASSERT_NOT_REACHED();
}

enum fileMode crGetFileMode(void) {
	return grenderer->prefs.fileMode;
}

void crSetRenderOrder(void) {
	ASSERT_NOT_REACHED();
}

void crGetRenderOrder(void) {
	ASSERT_NOT_REACHED();
}

void crSetThreadCount(int threadCount, bool fromSystem) {
	ASSERT_NOT_REACHED();
}

int crGetThreadCount(void) {
	return grenderer->prefs.threadCount;
}

void crSetSampleCount(int sampleCount) {
	ASSERT_NOT_REACHED();
}

int crGetSampleCount(void) {
	return grenderer->prefs.sampleCount;
}

void crSetBounces(int bounces) {
	ASSERT_NOT_REACHED();
}

int crGetBounces(void) {
	return grenderer->prefs.bounces;
}

void crSetTileWidth(int width) {
	ASSERT_NOT_REACHED();
}

int crGetTileWidth(void) {
	return grenderer->prefs.tileWidth;
}

void crSetTileHeight(int height) {
	ASSERT_NOT_REACHED();
}

int crGetTileHeight(void) {
	return grenderer->prefs.tileHeight;
}

void crSetImageWidth(int width) {
	ASSERT_NOT_REACHED();
}

int crGetImageWidth(void) {
	return grenderer->prefs.imageWidth;
}

void crSetImageHeight(int height) {
	ASSERT_NOT_REACHED();
}

int crGetImageHeight() {
	return grenderer->prefs.imageHeight;
}

void crSetFilePath(char *filePath) {
	ASSERT_NOT_REACHED();
}

char *crGetFilePath() {
	return grenderer->prefs.imgFilePath;
}

void crSetFileName(char *fileName) {
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
	time(&grenderer->state.start);
	currentImage = renderFrame(grenderer);
	time(&grenderer->state.stop);
	printDuration(difftime(grenderer->state.stop, grenderer->state.start));
}

//Interactive mode
void crStartInteractive(void);
void crPauseInteractive(void); //Toggle paused state
void crGetCurrentImage(void); //Just get the current buffer

void crTransformMesh(void); //Transform, recompute kd-tree, restart

void crMoveCamera(void/*struct dimension delta*/);
void crSetHDR(void);
