//
//  c-ray.h
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Cray public-facing API

void loadSceneFromFile(char *filePath);
void loadSceneFromBuf(char *buf);

void loadMeshFromFile(char *filePath);
void loadMeshFromBuf(char *buf);

//Preferences
void setFileMode(void);
void setRenderOrder(void);
void setThreadCount(int threadCount, bool fromSystem);
void setSampleCount(int sampleCount);
void setBounces(int bounces);
void setTileWidth(int width);
void setTileHeight(int height);
void setImageWidth(int width);
void setImageHeight(int height);
void setFilePath(char *filePath);
void setFileName(char *fileName);
void setAntialiasing(bool on);

//Single frame
void renderFrame(void);

//Interactive mode
void startInteractive(void);

void moveCamera(void/*struct dimension delta*/);
void setHDR(void);


