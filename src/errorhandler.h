//
//  errorhandler.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 14/09/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum logType {
	error,
	info,
	warning
};

void logr(enum logType type, const char *fmt, ...);
