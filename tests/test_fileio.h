//
//  test_fileio.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/09/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/utils/fileio.h"

bool fileio_humanFileSize(void) {
	
	char *humanSize = NULL;
	
	humanSize = human_file_size(600, NULL);
	test_assert(stringEquals(humanSize, "600B"));
	free(humanSize);
	
	humanSize = human_file_size(1000, NULL);
	test_assert(stringEquals(humanSize, "1.00kB"));
	free(humanSize);
	
	humanSize = human_file_size(1000 * 1000, NULL);
	test_assert(stringEquals(humanSize, "1.00MB"));
	free(humanSize);
	
	humanSize = human_file_size(1000 * 1000 * 1000, NULL);
	test_assert(stringEquals(humanSize, "1.00GB"));
	free(humanSize);
	
	humanSize = human_file_size((unsigned long)1000 * 1000 * 1000 * 1000, NULL);
	test_assert(stringEquals(humanSize, "1.00TB"));
	free(humanSize);
	
	return true;
}

bool fileio_getFileName(void) {
	
	char *fileName = get_file_name("/Users/vkoskiv/c-ray/bin/c-ray");
	test_assert(stringEquals(fileName, "c-ray"));
	free(fileName);
	
	return true;
}

bool fileio_getFilePath(void) {
	
	char *fileName = get_file_path("/Users/vkoskiv/c-ray/bin/c-ray");
	test_assert(stringEquals(fileName, "/Users/vkoskiv/c-ray/bin/"));
	free(fileName);
	
	return true;
}
