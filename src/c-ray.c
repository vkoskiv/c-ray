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
#include "utils/platform/terminal.h"
#include "utils/assert.h"
#include "datatypes/image/texture.h"
#include "utils/ui.h"
#include "utils/timer.h"
#include "utils/args.h"
#include "utils/encoders/encoder.h"
#include <stdarg.h>

#define VERSION "0.6.3"

//Internal renderer state
struct renderer *grenderer = NULL;

struct texture *currentImage = NULL;

char *crGetVersion() {
	return VERSION;
}

char *crGitHash() {
	return gitHash();
}

void crInitTerminal() {
	initTerminal();
}

void crRestoreTerminal() {
	restoreTerminal();
}

void crParseOptions(int argc, char **argv) {
	parseOptions(argc, argv);
}

bool crOptionIsSet(char *key) {
	return isSet(key);
}

char *crPathArg() {
	return pathArg();
}

void crDestroyOptions() {
	destroyOptions();
}

char *crGetFilePath(char *fullPath) {
	return getFilePath(fullPath);
}

void crInitSDL() {
#ifdef UI_ENABLED
	if (grenderer->mainDisplay->enabled) {
		ASSERT(!grenderer->mainDisplay->window);
		initSDL(grenderer->mainDisplay);
	}
#endif
}

void crDestroySDL() {
#ifdef UI_ENABLED
	if (grenderer->mainDisplay->enabled) {
		ASSERT(grenderer->mainDisplay->window);
		destroyDisplay(grenderer->mainDisplay);
	}
#endif
}

void crWriteImage() {
	if (currentImage) {
		if (grenderer->state.saveImage) {
			writeImage(currentImage, (struct renderInfo){
				.bounces = crGetBounces(),
				.samples = crGetSampleCount(),
				.crayVersion = crGetVersion(),
				.gitHash = crGitHash(),
				.renderTime = getMs(*grenderer->state.timer),
				.threadCount = crGetThreadCount()
			});
		} else {
			logr(info, "Abort pressed, image won't be saved.\n");
		}
	}
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
	destroyRenderer(grenderer);
	destroyTexture(currentImage);
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

void crLoadMeshFromFile(char *filePath) {
	(void)filePath;
	ASSERT_NOT_REACHED();
}

void crLoadMeshFromBuf(char *buf) {
	(void)buf;
	ASSERT_NOT_REACHED();
}

int crLoadSceneFromBuf(char *buf) {
	return loadScene(grenderer, buf);
}

void crLog(const char *fmt, ...) {
	char buf[512];
	va_list vl;
	va_start(vl, fmt);
	vsnprintf(buf, sizeof(buf), fmt, vl);
	va_end(vl);
	logr(info, "%s", buf);
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
	ASSERT(sampleCount > 0);
	grenderer->prefs.sampleCount = sampleCount;
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

void crSetOutputPath(char *filePath) {
	grenderer->prefs.imgFilePath = filePath;
}

char *crGetOutputPath() {
	return grenderer->prefs.imgFilePath;
}

void crSetFileName(char *fileName) {
	(void)fileName;
	ASSERT_NOT_REACHED();
}

char *crGetFileName() {
	return grenderer->prefs.imgFileName;
}

void crSetAssetPath(char *assetPath) {
	grenderer->prefs.assetPath = assetPath;
}

char *crGetAssetPath(void) {
	return grenderer->prefs.assetPath;
}

void crSetAntialiasing(bool on) {
	grenderer->prefs.antialiasing = on;
}

bool crGetAntialiasing() {
	return grenderer->prefs.antialiasing;
}

void crRenderSingleFrame() {
	crInitSDL();
	startTimer(grenderer->state.timer);
	currentImage = renderFrame(grenderer);
	printDuration(getMs(*grenderer->state.timer));
	crDestroySDL();
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
