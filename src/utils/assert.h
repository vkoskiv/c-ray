//
//  assert.h
//  C-ray
//
//  Created by Valtteri on 11.12.2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

void assertFailed(const char *file, const char *func, int line, const char *expr) {
	printf("ASSERTION FAILED: In %s in function %s on line %i, expression \"%s\"", file, func, line, expr);
	abort();
}

#define ASSERT(expr) \
	if (!(expr)) \
		assertFailed(__FILE__, __FUNCTION__, __LINE__, #expr)

#define ASSERT_NOT_REACHED() \
	assertFailed(__FILE__, __FUNCTION__, __LINE__, "ASSERT_NOT_REACHED")
