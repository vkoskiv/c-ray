//
//  mutex.h
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Platform-agnostic mutexes

struct crMutex;

struct crMutex *createMutex(void);

void lockMutex(struct crMutex *m);

void releaseMutex(struct crMutex *m);
