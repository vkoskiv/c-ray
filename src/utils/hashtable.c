//
//  hashtable.c
//  C-ray
//
//  Created by Valtteri on 17.11.2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "hashtable.h"

#include "../utils/logging.h"
#include "../datatypes/vector.h"
#include "assert.h"
#include "../utils/filehandler.h"
#include "../utils/string.h"

// Fowler-Noll-Vo hash function
uint64_t hashDataToU64(const char *data, size_t size) {
	static const uint64_t FNVOffsetBasis = 14695981039346656037U;
	static const uint64_t FNVPrime = 1099511628211;
	uint64_t hash = FNVOffsetBasis;
	for (size_t i = 0; i < size; ++i) {
		char dataByte = data[i];
		hash = (~0xFF & hash) | (0xFF & (hash ^ dataByte));
		hash = hash * FNVPrime;
	}
	return hash;
}

struct bucket *getBucketPtr(struct hashtable *e, const char *key) {
	struct bucket *result;
	uint64_t index = hashDataToU64(key, strlen(key)) % e->size;
	for (; index < e->size; ++index) {
		result = &e->data[index];
		if (result->used && result->key != NULL) {
			if (!strcmp(key, result->key)) return result;
		} else {
			return result;
		}
	}
	return NULL;
}

bool exists(struct hashtable *e, const char *key) {
	return getBucketPtr(e, key)->used;
}

void setVector(struct hashtable *e, const char *key, struct vector value) {
	struct bucket *ptr = getBucketPtr(e, key);
	if (!ptr->used)
		ptr->value = malloc(sizeof(struct vector));
	ptr->used = true;
	*(struct vector*)ptr->value = value;
}

struct vector getVector(struct hashtable *e, const char *key) {
	struct bucket *ptr = getBucketPtr(e, key);
	return ptr->used ? *(struct vector*)ptr->value : (struct vector){0.0f, 0.0f, 0.0f};
}

void setFloat(struct hashtable *e, const char *key, float value) {
	struct bucket *ptr = getBucketPtr(e, key);
	if (!ptr->used)
		ptr->value = malloc(sizeof(float));
	ptr->used = true;
	*(float*)ptr->value = value;
}

float getFloat(struct hashtable *e, const char *key) {
	return *(float*)getBucketPtr(e, key)->value;
}

void setTag(struct hashtable *e, const char *key) {
	struct bucket *ptr = getBucketPtr(e, key);
	ptr->used = true;
}

void setString(struct hashtable *e, const char *key, const char *value, int len) {
	ASSERT((int)strlen(value) == len);
	struct bucket *ptr = getBucketPtr(e, key);
	if (!ptr->used) {
		ptr->value = malloc(len * sizeof(char));
		copyString(value, (char**)&(ptr->value));
	}
	ptr->used = true;
}

char *getString(struct hashtable *e, const char *key) {
	struct bucket *ptr = getBucketPtr(e, key);
	return ptr->used ? (char*)ptr->value : NULL;
}

void setInt(struct hashtable *e, const char *key, int value) {
	struct bucket *ptr = getBucketPtr(e, key);
	if (ptr->used) free(ptr->value);
	ptr->value = malloc(sizeof(int));
	ptr->used = true;
	*(int*)ptr->value = value;
}

int getInt(struct hashtable *e, const char *key) {
	return *(int*)getBucketPtr(e, key)->value;
}

static const uint64_t defaultTableSize = 256;
struct hashtable *newTable() {
	struct hashtable *table = malloc(sizeof(*table));
	table->size = defaultTableSize;
	table->data = malloc(sizeof(*table->data) * defaultTableSize);
	for (uint64_t i = 0; i < defaultTableSize; ++i) {
		table->data[i].used = false;
		table->data[i].value = NULL;
		table->data[i].key = NULL;
	}
	return table;
}

void freeTable(struct hashtable *table) {
	if (!table) return;
	
	for (uint64_t i = 0; i < defaultTableSize; ++i) {
		if (table->data[i].used) {
			free(table->data[i].value);
			free(table->data[i].key);
		}
	}
	free(table->data);
	free(table);
}

void printTableUsage(struct hashtable *t) {
	for (uint64_t i = 0; i < t->size; ++i) {
		printf("[%s]", t->data[i].used ? "x" : " ");
	}
	printf("\n");
}

void testTable() {
	struct hashtable *table = newTable();
	
	printTableUsage(table);
	
	setFloat(table, "Foo", 0.1f);
	setFloat(table, "Bar", 0.2f);
	setFloat(table, "Baz", 0.3f);
	
	printTableUsage(table);
	
	logr(debug, "Value at %s is %f\n", "Foo", getFloat(table, "Foo"));
	logr(debug, "Value at %s is %f\n", "Bar", getFloat(table, "Bar"));
	logr(debug, "Value at %s is %f\n", "Baz", getFloat(table, "Baz"));
	
	setFloat(table, "Baz", 0.4f);
	logr(debug, "Value at %s is %f\n", "Baz", getFloat(table, "Baz"));
	/*printUsage(table);
	for (int i = 0; i < 10000000; ++i) {
		setFloat(table, "Yeet", 3.3333);
	}
	printUsage(table);
	*/
	logr(debug, "Testing overfill\n");
	char buf[5];
	for (uint64_t i = 0; i < defaultTableSize; ++i) {
		sprintf(buf, "%llu", i);
		setFloat(table, buf, (float)i);
		printTableUsage(table);
	}
	
	freeTable(table);
}
