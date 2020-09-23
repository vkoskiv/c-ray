//
//  test_hashtable.h
//  C-ray
//
//  Created by Valtteri on 17.9.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/utils/hashtable.h"
#include "../src/datatypes/vector.h"

bool hashtable_mixed(void) {
	bool pass = true;
	
	struct hashtable *table = newTable();
	
	setVector(table, "key0", (struct vector){1.0f, 2.0f, 3.0f});
	setFloat(table, "key1", 123.4f);
	setTag(table, "key2");
	setString(table, "key3", "This is my cool string");
	setInt(table, "key4", 1234);
	
	test_assert(table);
	test_assert(vecEquals(getVector(table, "key0"), (struct vector){1.0f, 2.0f, 3.0f}));
	test_assert(getFloat(table, "key1") == 123.4f);
	test_assert(exists(table, "key2"));
	test_assert(stringEquals(getString(table, "key3"), "This is my cool string"));
	test_assert(getInt(table, "key4") == 1234);
	
	freeTable(table);
	
	return pass;
}

uint64_t used_count(struct hashtable *t) {
	uint64_t used = 0;
	for (uint64_t i = 0; i < t->size; ++i) {
		if (t->data[i].used) used++;
	}
	return used;
}

bool hashtable_fill(void) {
	bool pass = true;
	
	struct hashtable *table = newTable();
	
	char buf[20];
	
	// Collides at i == 203
	for (uint64_t i = 0; i < 200 && pass; ++i) {
		test_assert(used_count(table) == i);
		sprintf(buf, "key%llu", i);
		setTag(table, buf);
	}
	
	freeTable(table);
	
	return pass;
}
