//
//  logging.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 14/09/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

enum logType {
	error,
	info,
	warning
};

void logr(enum logType type, const char *fmt, ...);

void printStats(struct renderer *r, unsigned long long ms, unsigned long long samples, int thread);
