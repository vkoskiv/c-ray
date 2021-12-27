//
//  test_hashtable.h
//  C-ray
//
//  Created by Valtteri on 17.9.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>

#include "../src/utils/hashtable.h"
#include "../src/datatypes/vector.h"

//TODO: Consider having some of these common comparators included with the hashtable API.
bool compare(const void *A, const void *B) {;
	const char *a = (const char *)A;
	const char *b = (const char *)B;
	return stringEquals(a, b);
}

bool hashtable_doubleInsert(void) {
	struct hashtable *table = newHashtable(compare, NULL);
	
	float firstValue = 12;
	char *firstKey = "twelve";
	insertInHashtable(table, &firstValue, sizeof(float), hashString(hashInit(), firstKey));
	
	float *got = findInHashtable(table, "twelve", hashString(hashInit(), "twelve"));
	test_assert(got);
	
	test_assert(*got == 12);
	return true;
}

bool hashtable_mixed(void) {
	struct constantsDatabase *database = newConstantsDatabase();
	
	setDatabaseVector(database, "key0", (struct vector){1.0f, 2.0f, 3.0f});
	setDatabaseFloat(database, "key1", 123.4f);
	setDatabaseTag(database, "key2");
	setDatabaseString(database, "key3", "This is my cool string");
	setDatabaseInt(database, "key4", 1234);
	
	test_assert(database);
	test_assert(vecEquals(getDatabaseVector(database, "key0"), (struct vector){1.0f, 2.0f, 3.0f}));
	test_assert(getDatabaseFloat(database, "key1") == 123.4f);
	test_assert(existsInDatabase(database, "key2"));
	test_assert(stringEquals(getDatabaseString(database, "key3"), "This is my cool string"));
	test_assert(getDatabaseInt(database, "key4") == 1234);
	
	freeConstantsDatabase(database);
	return true;
}

bool hashtable_fill(void) {
	struct constantsDatabase *database = newConstantsDatabase();
	char buf[20];
	size_t iterCount = 10000;
	for (size_t i = 0; i < iterCount; ++i) {
		sprintf(buf, "key%lu", i);
		setDatabaseInt(database, buf, (int)i);
	}
	test_assert(database->hashtable.elemCount == iterCount);
	for (size_t i = 0; i < iterCount; ++i) {
		sprintf(buf, "key%lu", i);
		test_assert((size_t)getDatabaseInt(database, buf) == i);
	}
	freeConstantsDatabase(database);
	return true;
}
