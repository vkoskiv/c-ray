//
//  cr_assert.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 11.12.2019.
//  Copyright © 2019-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

void assertFailed(const char *file, const char *func, int line, const char *expr);

#ifdef CRAY_DEBUG_ENABLED

#define ASSERT(expr) \
	if ((expr)) \
		{} \
	else \
		assertFailed(__FILE__, __FUNCTION__, __LINE__, #expr)

#define ASSERT_NOT_REACHED() \
	assertFailed(__FILE__, __FUNCTION__, __LINE__, "ASSERT_NOT_REACHED")

#else

#define ASSERT(expr)

#define ASSERT_NOT_REACHED()

#endif
