//
//  assert.c
//  C-ray
//
//  Created by Valtteri on 28.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "assert.h"

void assertFailed(const char *file, const char *func, int line, const char *expr) {
	printf("ASSERTION FAILED: In %s in function %s on line %i, expression \"%s\"\n", file, func, line, expr);
	abort();
}
