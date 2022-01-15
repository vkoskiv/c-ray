//
//  c-ray.h
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Cray public-facing API

struct renderInfo;
struct texture;
struct renderer;

//Utilities
char *crGetVersion(void); //The current semantic version

char *crGitHash(void); //The current git hash of the build

bool isDebug(void);

void crInitialize(void); //Run initial setup of the environment

void crParseArgs(int argc, char **argv);
bool crOptionIsSet(char *key);
char *crPathArg(void);
void crDestroyOptions(void);

char *crGetFilePath(char *fullPath);

void crWriteImage(struct renderer *r); //Write out the current image to file

char *crReadFile(size_t *bytes);
char *crReadStdin(size_t *bytes);

struct renderer *cr_new_renderer(void);
void cr_destroy_renderer(struct renderer *r);

int crLoadSceneFromFile(struct renderer *r, char *filePath);
int crLoadSceneFromBuf(struct renderer *r, char *buf);

void crLoadMeshFromFile(char *filePath);
void crLoadMeshFromBuf(char *buf);

void crLog(const char *fmt, ...)
#ifndef WINDOWS
__attribute__ ((format (printf, 1, 2)))
#endif
;

//Preferences
void crSetRenderOrder(void);
void crGetRenderOrder(void);

void crSetThreadCount(struct renderer *r, int threadCount, bool fromSystem);
int crGetThreadCount(struct renderer *r);

void crSetSampleCount(struct renderer *r, int sampleCount);
int crGetSampleCount(struct renderer *r);

void crSetBounces(struct renderer *r, int bounces);
int crGetBounces(struct renderer *r);

void crSetTileWidth(struct renderer *r, unsigned width);
unsigned crGetTileWidth(struct renderer *r);

void crSetTileHeight(struct renderer *r, unsigned height);
unsigned crGetTileHeight(struct renderer *r);

void crSetImageWidth(struct renderer *r, unsigned width);
unsigned crGetImageWidth(struct renderer *r);

void crSetImageHeight(struct renderer *r, unsigned height);
unsigned crGetImageHeight(struct renderer *r);

void crSetOutputPath(struct renderer *r, char *filePath);
char *crGetOutputPath(struct renderer *r);

void crSetFileName(struct renderer *r, char *fileName);
char *crGetFileName(struct renderer *r);

void crSetAssetPath(struct renderer *r);
char *crGetAssetPath(struct renderer *r);

//Single frame
void crStartRenderer(struct renderer *);

//Network render worker
void crStartRenderWorker(void);

//Interactive mode
void crStartInteractive(void);
void crPauseInteractive(void); //Toggle paused state
void crGetCurrentImage(void); //Just get the current buffer
void crRestartInteractive(void);

void crTransformMesh(void); //Transform, recompute kd-tree, restart

void crMoveCamera(void/*struct dimension delta*/);
void crSetHDR(void);

