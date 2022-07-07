//
//  mutex.h
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Platform-agnostic mutexes

struct cr_mutex;

struct cr_mutex *mutex_create(void);

void mutex_lock(struct cr_mutex *m);

void mutex_release(struct cr_mutex *m);
