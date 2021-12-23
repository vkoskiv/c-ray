//
//  c-ray.c
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "c-ray.h"

#include <stdarg.h>
#include <stdio.h>

#include "datatypes/image/imagefile.h"
#include "renderer/renderer.h"
#include "datatypes/scene.h"
#include "utils/gitsha1.h"
#include "utils/logging.h"
#include "utils/fileio.h"
#include "utils/platform/terminal.h"
#include "utils/assert.h"
#include "datatypes/image/texture.h"
#include "utils/ui.h"
#include "utils/timer.h"
#include "utils/args.h"
#include "utils/encoders/encoder.h"
#include "utils/string.h"
#include "utils/protocol/server.h"
#include "utils/protocol/worker.h"
#include "utils/filecache.h"
#include "utils/hashtable.h"
#include "datatypes/camera.h"

#define VERSION "0.6.3"

//Internal renderer state
struct renderer *g_renderer = NULL;

struct texture *currentImage = NULL;

char *crGetVersion() {
	return VERSION;
}

char *crGitHash() {
	return gitHash();
}

bool isDebug() {
#ifdef CRAY_DEBUG_ENABLED
	return true;
#else
	return false;
#endif
}

void atExit() {
	restoreTerminal();
}

void crInitialize() {
	initTerminal();
	atexit(atExit);
}

void crParseArgs(int argc, char **argv) {
	parseArgs(argc, argv);
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

void crWriteImage() {
	if (currentImage) {
		if (g_renderer->state.saveImage) {
			struct imageFile *file = newImageFile(currentImage, g_renderer->prefs.imgFilePath, g_renderer->prefs.imgFileName, g_renderer->prefs.imgCount, g_renderer->prefs.imgType);
			file->info = (struct renderInfo){
				.bounces = crGetBounces(),
				.samples = crGetSampleCount(),
				.crayVersion = crGetVersion(),
				.gitHash = crGitHash(),
				.renderTime = getMs(*g_renderer->state.timer),
				.threadCount = crGetThreadCount()
			};
			writeImage(file);
			destroyImageFile(file);
		} else {
			logr(info, "Abort pressed, image won't be saved.\n");
		}
	}
}

char *crReadFile(size_t *bytes) {
	return loadFile(pathArg(), bytes);
}

char *crReadStdin(size_t *bytes) {
	return readStdin(bytes);
}

void crInitRenderer() {
	ASSERT(!g_renderer);
	g_renderer = newRenderer();
	crSetAssetPath();
}

void crDestroyRenderer() {
	ASSERT(g_renderer);
	destroyRenderer(g_renderer);
	g_renderer = NULL;
}

int crLoadSceneFromFile(char *filePath) {
	size_t bytes = 0;
	char *input = loadFile(filePath, &bytes);
	if (input) {
		if (loadScene(g_renderer, filePath) != 0) {
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
	return loadScene(g_renderer, buf);
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
	g_renderer->prefs.threadCount = threadCount;
	g_renderer->prefs.fromSystem = fromSystem;
	crRestartInteractive();
}

int crGetThreadCount(void) {
	return g_renderer->prefs.threadCount;
}

void crSetSampleCount(int sampleCount) {
	ASSERT(sampleCount > 0);
	g_renderer->prefs.sampleCount = sampleCount;
}

int crGetSampleCount(void) {
	return g_renderer->prefs.sampleCount;
}

void crSetBounces(int bounces) {
	ASSERT(bounces > 0);
	g_renderer->prefs.bounces = bounces;
}

int crGetBounces(void) {
	return g_renderer->prefs.bounces;
}

unsigned crGetTileWidth(void) {
	return g_renderer->prefs.tileWidth;
}

unsigned crGetTileHeight(void) {
	return g_renderer->prefs.tileHeight;
}

unsigned crGetImageWidth(void) {
	return g_renderer->scene->cameras[g_renderer->prefs.selected_camera].width;
}

unsigned crGetImageHeight() {
	return g_renderer->scene->cameras[g_renderer->prefs.selected_camera].height;
}

void crSetOutputPath(char *filePath) {
	g_renderer->prefs.imgFilePath = filePath;
}

char *crGetOutputPath() {
	return g_renderer->prefs.imgFilePath;
}

void crSetFileName(char *fileName) {
	(void)fileName;
	ASSERT_NOT_REACHED();
}

char *crGetFileName() {
	return g_renderer->prefs.imgFileName;
}

void crSetAssetPath(void) {
	g_renderer->prefs.assetPath = crOptionIsSet("inputFile") ? crGetFilePath(crPathArg()) : crOptionIsSet("asset_path") ? specifiedAssetPath() : stringCopy("./");
}

char *crGetAssetPath(void) {
	return g_renderer->prefs.assetPath;
}

void crStartRenderer() {
	if (isSet("use_clustering")) {
		g_renderer->prefs.useClustering = true;
		g_renderer->state.clients = syncWithClients(g_renderer, &g_renderer->state.clientCount);
		free(g_renderer->sceneCache);
		g_renderer->sceneCache = NULL;
		destroyFileCache();
	}
	struct camera cam = g_renderer->scene->cameras[g_renderer->prefs.selected_camera];
	initDisplay(g_renderer->prefs.fullscreen, g_renderer->prefs.borderless, cam.width, cam.height, g_renderer->prefs.scale);
	startTimer(g_renderer->state.timer);
	currentImage = renderFrame(g_renderer);
	printDuration(getMs(*g_renderer->state.timer));
	destroyDisplay();
}

void crStartRenderWorker() {
	startWorkerServer();
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
