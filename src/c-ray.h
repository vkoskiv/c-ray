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

char *crGitHash(int chars); //The current git hash of the build

void crInitTerminal(void);
void crInitSDL(void);
void crDestroySDL(void);

struct renderInfo;
struct texture;
void crWriteImage(void);

char *crLoadFile(char *filePath, size_t *bytes);
char *crReadStdin(void);

void crInitRenderer(void);
struct display *crGetDisplay(void);
void crDestroyRenderer(void);

int crLoadSceneFromFile(char *filePath);
int crLoadSceneFromBuf(char *buf);

void crLoadMeshFromFile(char *filePath);
void crLoadMeshFromBuf(char *buf);

//Preferences
void crSetRenderOrder(void);
void crGetRenderOrder(void);

void crSetThreadCount(int threadCount, bool fromSystem);
int crGetThreadCount(void);

void crSetSampleCount(int sampleCount);
int crGetSampleCount(void);

void crSetBounces(int bounces);
int crGetBounces(void);

void crSetTileWidth(int width);
int crGetTileWidth(void);

void crSetTileHeight(int height);
int crGetTileHeight(void);

void crSetImageWidth(int width);
int crGetImageWidth(void);

void crSetImageHeight(int height);
int crGetImageHeight(void);

void crSetFilePath(char *filePath);
char *crGetFilePath(void);

void crSetFileName(char *fileName);
char *crGetFileName(void);

void crSetAntialiasing(bool on);
bool crGetAntialiasing(void);

//Single frame
void crRenderSingleFrame(void);

//Interactive mode
void crStartInteractive(void);
void crPauseInteractive(void); //Toggle paused state
void crGetCurrentImage(void); //Just get the current buffer

void crTransformMesh(void); //Transform, recompute kd-tree, restart

void crMoveCamera(void/*struct dimension delta*/);
void crSetHDR(void);

