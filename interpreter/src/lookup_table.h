#ifndef xanadu_lookup_table_h
#define xanadu_lookup_table_h

#include "common.h"
#include "value.h"

// Table entry ifnormation
typedef struct {
	ObjString *key;
	Value value;
} Entry;

// Table meta data
typedef struct {
	int count;
	int capacity;
	Entry *entries;
} Table;

// Initialise look up table
void init_table(Table *table);
// Free's used memory and resets table to initiale state
void freeTable(Table *table);
// Add object into loopup table
bool insert_into_table(Table *table, ObjString *key, Value value);
// Copy contents of one table into another
void table_add_all(Table *from, Table *to);
// Get value from table
bool table_get_from_table(Table *table, ObjString *key, Value *value);
// Delete value from table
bool table_delete_from_table(Table *table, ObjString *key);

#endif
