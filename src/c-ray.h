//
//  c-ray.h
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Cray public-facing API

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

struct renderInfo;
struct texture;
void crWriteImage(void); //Write out the current image

char *crReadFile(size_t *bytes);
char *crReadStdin(size_t *bytes);

void crInitRenderer(void);
void crDestroyRenderer(void);

int crLoadSceneFromFile(char *filePath);
int crLoadSceneFromBuf(char *buf);

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

void crSetThreadCount(int threadCount, bool fromSystem);
int crGetThreadCount(void);

void crSetSampleCount(int sampleCount);
int crGetSampleCount(void);

void crSetBounces(int bounces);
int crGetBounces(void);

void crSetTileWidth(unsigned width);
unsigned crGetTileWidth(void);

void crSetTileHeight(unsigned height);
unsigned crGetTileHeight(void);

void crSetImageWidth(unsigned width);
unsigned crGetImageWidth(void);

void crSetImageHeight(unsigned height);
unsigned crGetImageHeight(void);

void crSetOutputPath(char *filePath);
char *crGetOutputPath(void);

void crSetFileName(char *fileName);
char *crGetFileName(void);

void crSetAssetPath(void);
char *crGetAssetPath(void);

void crSetAntialiasing(bool on);
bool crGetAntialiasing(void);

//Single frame
void crRenderSingleFrame(void);

//Interactive mode
void crStartInteractive(void);
void crPauseInteractive(void); //Toggle paused state
void crGetCurrentImage(void); //Just get the current buffer
void crRestartInteractive(void);

void crTransformMesh(void); //Transform, recompute kd-tree, restart

void crMoveCamera(void/*struct dimension delta*/);
void crSetHDR(void);

