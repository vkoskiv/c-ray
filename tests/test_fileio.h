//
//  test_fileio.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/09/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/utils/fileio.h"

bool fileio_humanFileSize(void) {
	bool pass = true;
	
	char *humanSize = NULL;
	
	humanSize = humanFileSize(600);
	test_assert(stringEquals(humanSize, "600B"));
	free(humanSize);
	
	humanSize = humanFileSize(1000);
	test_assert(stringEquals(humanSize, "1.00kB"));
	free(humanSize);
	
	humanSize = humanFileSize(1000 * 1000);
	test_assert(stringEquals(humanSize, "1.00MB"));
	free(humanSize);
	
	humanSize = humanFileSize(1000 * 1000 * 1000);
	test_assert(stringEquals(humanSize, "1.00GB"));
	free(humanSize);
	
	humanSize = humanFileSize((unsigned long)1000 * 1000 * 1000 * 1000);
	test_assert(stringEquals(humanSize, "1.00TB"));
	free(humanSize);
	
	return pass;
}

bool fileio_getFileName(void) {
	bool pass = true;
	
	char *fileName = getFileName("/Users/vkoskiv/c-ray/bin/c-ray");
	test_assert(stringEquals(fileName, "c-ray"));
	free(fileName);
	
	return pass;
}

bool fileio_getFilePath(void) {
	bool pass = true;
	
	char *fileName = getFilePath("/Users/vkoskiv/c-ray/bin/c-ray");
	test_assert(stringEquals(fileName, "/Users/vkoskiv/c-ray/bin/"));
	free(fileName);
	
	return pass;
}
