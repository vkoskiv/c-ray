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

//Single frame
void renderFrame(void);

//Interactive mode
void startInteractive(void);


