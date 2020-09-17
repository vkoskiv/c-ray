//
//  hashtable.h
//  C-ray
//
//  Created by Valtteri on 17.11.2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct vector;

struct bucket {
	bool used;
	char *key;
	void *value;
};

struct hashtable {
	struct bucket *data;
	uint64_t size;
};

struct bucket *getBucketPtr(struct hashtable *e, const char *key);

bool exists(struct hashtable *e, const char *key);

void setVector(struct hashtable *e, const char *key, struct vector value);

struct vector getVector(struct hashtable *e, const char *key);

void setFloat(struct hashtable *e, const char *key, float value);

float getFloat(struct hashtable *e, const char *key);

//No data is stored. This key is just occupied, and can be checked for with exists()
void setTag(struct hashtable *e, const char *key);

void setString(struct hashtable *e, const char *key, const char *value);

char *getString(struct hashtable *e, const char *key);

void setInt(struct hashtable *e, const char *key, int value);

int getInt(struct hashtable *e, const char *key);

struct hashtable *newTable(void);

void freeTable(struct hashtable *table);

void testTable(void);
