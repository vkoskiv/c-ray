//
//  c-ray.c
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
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

void crWriteImage(struct renderer *r) {
	if (currentImage) {
		if (r->state.saveImage) {
			struct imageFile *file = newImageFile(currentImage, r->prefs.imgFilePath, r->prefs.imgFileName, r->prefs.imgCount, r->prefs.imgType);
			file->info = (struct renderInfo){
				.bounces = crGetBounces(r),
				.samples = crGetSampleCount(r),
				.crayVersion = crGetVersion(),
				.gitHash = crGitHash(),
				.renderTime = getMs(r->state.timer),
				.threadCount = crGetThreadCount(r)
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

struct renderer *cr_new_renderer() {
	struct renderer *r = newRenderer();
	crSetAssetPath(r);
	return r;
}

void cr_destroy_renderer(struct renderer *r) {
	ASSERT(r);
	destroyRenderer(r);
}

int crLoadSceneFromFile(struct renderer *r, char *filePath) {
	size_t bytes = 0;
	char *input = loadFile(filePath, &bytes);
	if (input) {
		if (loadScene(r, filePath) != 0) {
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

int crLoadSceneFromBuf(struct renderer *r, char *buf) {
	return loadScene(r, buf);
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

void crSetThreadCount(struct renderer *r, int threadCount, bool fromSystem) {
	r->prefs.threadCount = threadCount;
	r->prefs.fromSystem = fromSystem;
	crRestartInteractive();
}

int crGetThreadCount(struct renderer *r) {
	return r->prefs.threadCount;
}

void crSetSampleCount(struct renderer *r, int sampleCount) {
	ASSERT(sampleCount > 0);
	r->prefs.sampleCount = sampleCount;
}

int crGetSampleCount(struct renderer *r) {
	return r->prefs.sampleCount;
}

void crSetBounces(struct renderer *r, int bounces) {
	ASSERT(bounces > 0);
	r->prefs.bounces = bounces;
}

int crGetBounces(struct renderer *r) {
	return r->prefs.bounces;
}

unsigned crGetTileWidth(struct renderer *r) {
	return r->prefs.tileWidth;
}

unsigned crGetTileHeight(struct renderer *r) {
	return r->prefs.tileHeight;
}

unsigned crGetImageWidth(struct renderer *r) {
	return r->scene->cameras[r->prefs.selected_camera].width;
}

unsigned crGetImageHeight(struct renderer *r) {
	return r->scene->cameras[r->prefs.selected_camera].height;
}

void crSetOutputPath(struct renderer *r, char *filePath) {
	r->prefs.imgFilePath = filePath;
}

char *crGetOutputPath(struct renderer *r) {
	return r->prefs.imgFilePath;
}

void crSetFileName(struct renderer *r, char *fileName) {
	(void)r;
	(void)fileName;
	ASSERT_NOT_REACHED();
}

char *crGetFileName(struct renderer *r) {
	return r->prefs.imgFileName;
}

void crSetAssetPath(struct renderer *r) {
	r->prefs.assetPath = crOptionIsSet("inputFile") ? crGetFilePath(crPathArg()) : crOptionIsSet("asset_path") ? specifiedAssetPath() : stringCopy("./");
}

char *crGetAssetPath(struct renderer *r) {
	return r->prefs.assetPath;
}

void crStartRenderer(struct renderer *r) {
	if (isSet("use_clustering")) {
		r->prefs.useClustering = true;
		r->state.clients = syncWithClients(r, &r->state.clientCount);
		free(r->sceneCache);
		r->sceneCache = NULL;
		destroyFileCache();
	}
	struct camera cam = r->scene->cameras[r->prefs.selected_camera];
	initDisplay(r->prefs.fullscreen, r->prefs.borderless, cam.width, cam.height, r->prefs.scale);
	startTimer(&r->state.timer);
	currentImage = renderFrame(r);
	printDuration(getMs(r->state.timer));
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
