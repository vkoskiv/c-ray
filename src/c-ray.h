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

void crInitTerminal(void); //Disable output buffering and configure Windows terminals
void crRestoreTerminal(void);

char *crGetFilePath(char *fullPath);

struct renderInfo;
struct texture;
void crWriteImage(void); //Write out the current image

char *crLoadFile(char *filePath, size_t *bytes);
char *crReadStdin(size_t *bytes);

void crInitRenderer(void);
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

void crSetOutputPath(char *filePath);
char *crGetOutputPath(void);

void crSetFileName(char *fileName);
char *crGetFileName(void);

void crSetAssetPath(char *assetPath);
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

