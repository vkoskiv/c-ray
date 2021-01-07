//
//  hashtable.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 17.11.2019.
//  Copyright © 2019-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "memory.h"

struct vector;

struct bucket {
	struct bucket *next;
	uint32_t hash;
	// Since C99 does not have max_align_t, we have to create a type
	// that has the largest alignment requirement.
	cray_max_align_t data[];
};

struct hashtable {
	struct bucket **buckets;
	struct block **pool;
	size_t bucketCount;
	size_t elemCount;
	bool (*compare)(const void *, const void *);
};

// Hash functions (using FNV)
uint32_t hashInit(void);
uint32_t hashCombine(uint32_t, uint8_t);
uint32_t hashBytes(uint32_t, const void *, size_t);
uint32_t hashString(uint32_t, const char *);

struct hashtable *newHashtable(bool (*compare)(const void *, const void *), struct block **pool);
// Finds the given element in the hash table, using the hash value `hash`.
// Returns a pointer to the element if it was found, or NULL otherwise.
void *findInHashtable(struct hashtable *hashtable, const void *element, uint32_t hash);
// Inserts the given element in the hash table, using the hash value `hash`.
// Returns `true` if the insertion is a success (the element did not exist before), `false` otherwise.
bool insertInHashtable(struct hashtable *hashtable, const void *element, size_t elementSize, uint32_t hash);
// Inserts or replaces the given element in the hash table, using the hash value `hash`.
void replaceInHashtable(struct hashtable *hashtable, const void *element, size_t elementSize, uint32_t hash);
// Always inserts the element in the hash table, not caring for duplicates.
void forceInsertInHashtable(struct hashtable *hashtable, const void *element, size_t elementSize, uint32_t hash);
// Removes the given element from the hash table, using the hash value `hash`.
// Returns `true` if the removal is a success, `false` otherwise.
bool removeFromHashtable(struct hashtable *hashtable, const void *element, uint32_t hash);
void destroyHashtable(struct hashtable *hashtable);

// Database used by the scene to store constants.
struct constantsDatabase {
	struct hashtable hashtable;
};

struct constantsDatabase *newConstantsDatabase(void);
bool existsInDatabase(struct constantsDatabase *database, const char *key);
void setDatabaseVector(struct constantsDatabase *database, const char *key, struct vector value);
struct vector getDatabaseVector(struct constantsDatabase *database, const char *key);
void setDatabaseFloat(struct constantsDatabase *database, const char *key, float value);
float getDatabaseFloat(struct constantsDatabase *database, const char *key);
void setDatabaseString(struct constantsDatabase *database, const char *key, const char *value);
char *getDatabaseString(struct constantsDatabase *database, const char *key);
void setDatabaseInt(struct constantsDatabase *database, const char *key, int value);
int getDatabaseInt(struct constantsDatabase *database, const char *key);
// No data is stored. This key is just occupied, and can be checked for with existsInDatabase()
void setDatabaseTag(struct constantsDatabase *database, const char *key);
void freeConstantsDatabase(struct constantsDatabase *database);
