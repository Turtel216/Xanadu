// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#define TABLE_MAX_LOAD 0.75

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "lookup_table.h"
#include "value.h"

// Initialise look up table
void init_table(Table *table)
{
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

// Free's used memory and resets table to initiale state
void free_table(Table *table)
{
	FREE_ARRAY(Entry, table->entries, table->capacity);
	init_table(table);
}

// Copy contents of one table into another
void table_add_all(Table *from, Table *to)
{
	for (int i = 0; i < from->capacity; i++) {
		Entry *entry = &from->entries[i];
		if (entry->key != NULL) {
			insert_into_table(to, entry->key, entry->value);
		}
	}
}

// Find table entry
static Entry *find_entry(Entry *entries, int capacity, ObjString *key)
{
	uint32_t index = key->hash % capacity;
	Entry *tombstone = NULL;

	for (;;) {
		Entry *entry = &entries[index];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) {
				// Empty entry.
				return tombstone != NULL ? tombstone : entry;
			} else {
				// We found a tombstone.
				if (tombstone == NULL)
					tombstone = entry;
			}
		} else if (entry->key == key) {
			// We found the key.
			return entry;
		}

		index = (index + 1) % capacity;
	}
}

// Get value from table
bool table_get_from_table(Table *table, ObjString *key, Value *value)
{
	// Check for empty table
	if (table->count == 0)
		return false;

	// Look for value in table
	Entry *entry = find_entry(table->entries, table->capacity, key);
	// Nothing found
	if (entry->key == NULL)
		return false;

	// Assign found value to value refrence
	*value = entry->value;
	return true;
}

// Delete value from table
bool delete_from_table(Table *table, ObjString *key)
{
	if (table->count == 0)
		return false;

	// Find the entry.
	Entry *entry = find_entry(table->entries, table->capacity, key);
	// Nothing found
	if (entry->key == NULL)
		return false;

	// Place a tombstone in the entry.
	entry->key = NULL;
	entry->value = BOOL_VAL(true);
	return true;
}

// Update tables capacity
static void adjust_capacity(Table *table, int capacity)
{
	// Allocate memory of old table
	Entry *entries = ALLOCATE(Entry, capacity);
	for (int i = 0; i < capacity; ++i) {
		entries[i].key = NULL;
		entries[i].value = NIL_VAL;
	}

	// Adjust tombstone count
	table->count = 0;

	// Fix collisions
	for (int i = 0; i < table->capacity; ++i) {
		Entry *entry = &table->entries[i];
		if (entry->key == NULL)
			continue;

		Entry *dest = find_entry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
	}

	// Free memory of old table
	FREE_ARRAY(Entry, table->entries, table->capacity);

	table->entries = entries;
	table->capacity = capacity;
}

// Find String object in hash table
ObjString *table_find_string(Table *table, const char *chars, int length,
			     uint32_t hash)
{
	if (table->count == 0)
		return NULL;

	uint32_t index = hash % table->capacity;
	for (;;) {
		Entry *entry = &table->entries[index];
		if (entry->key == NULL) {
			// Stop if we find an empty non-tombstone entry.
			if (IS_NIL(entry->value))
				return NULL;
		} else if (entry->key->length == length &&
			   entry->key->hash == hash &&
			   memcmp(entry->key->chars, chars, length) == 0) {
			// We found it.
			return entry->key;
		}

		index = (index + 1) % table->capacity;
	}
}

// Add object into loopup table
bool insert_into_table(Table *table, ObjString *key, Value value)
{
	// Check if array fits new array
	// grow array if not
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		int capacity = GROW_CAPACITY(table->capacity);
		adjust_capacity(table, capacity);
	}

	// Insert entry
	Entry *entry = find_entry(table->entries, table->capacity, key);
	bool isNewKey = entry->key == NULL;
	if (isNewKey && IS_NIL(entry->value))
		table->count++;

	entry->key = key;
	entry->value = value;
	return isNewKey;
}

void table_remove_white(Table *table)
{
	for (int i = 0; i < table->capacity; i++) {
		Entry *entry = &table->entries[i];
		if (entry->key != NULL && !entry->key->obj.is_marked) {
			delete_from_table(table, entry->key);
		}
	}
}
