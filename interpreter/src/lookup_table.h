// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#ifndef xanadu_lookup_table_h
#define xanadu_lookup_table_h

#include "common.h"
#include "value.h"

// Table entry ifnormation
typedef struct {
	ObjString *key; // Entry object
	Value value; // Entry value
} Entry;

// Table meta data
typedef struct {
	int count; // Element count
	int capacity; // Capacity count
	Entry *entries; // Array of entries
} Table;

// Initialise look up table
void init_table(Table *table);
// Free's used memory and resets table to initiale state
void free_table(Table *table);
// Add object into loopup table
bool insert_into_table(Table *table, ObjString *key, Value value);
// Copy contents of one table into another
void table_add_all(Table *from, Table *to);
// Get value from table
bool table_get_from_table(Table *table, ObjString *key, Value *value);
// Delete value from table
bool delete_from_table(Table *table, ObjString *key);
// Find String object in hash table
ObjString *table_find_string(Table *table, const char *chars, int length,
			     uint32_t hash);

#endif
