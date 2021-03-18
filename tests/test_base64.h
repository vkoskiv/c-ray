//
//  test_base64.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 18/03/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "../src/utils/base64.h"
#include "../src/utils/string.h"
#include "../src/utils/fileio.h"

bool base64_basic(void) {
	char *original = "This is the original string right here.";
	char *encoded = b64encode(original, strlen(original));
	char *decoded = b64decode(encoded, strlen(encoded));
	test_assert(stringEquals(original, decoded));
	
	free(encoded);
	free(decoded);

	return true;
}
