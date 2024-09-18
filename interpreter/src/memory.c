// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#define GC_HEAP_GROW_FACTOR \
	2 // Factor by which the garbage collector heap grows

#include <stdlib.h>

#include "memory.h"
#include "error.h"
#include "object.h"
#include "vm.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

// Reallocate memory for a given pointer.
// Adjusts the allocated memory from oldSize to newSize.
// Updates the VM's bytesAllocated count and may trigger garbage collection
// if the new allocation exceeds the current threshold.
//
// Parameters:
//   pointer - Pointer to the memory to reallocate
//   oldSize - Previous size of the memory allocation
//   newSize - New size of the memory allocation
//
// Returns:
//   A pointer to the reallocated memory
void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
	vm.bytesAllocated += newSize - oldSize;

	if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
		// Force garbage collection for testing purposes
		collect_garbage();
#endif

		// Perform garbage collection if memory usage exceeds the threshold
		if (vm.bytesAllocated > vm.nextGC) {
			collect_garbage();
		}
	}

	if (newSize == 0) {
		free(pointer);
		return NULL;
	}

	void *result = realloc(pointer, newSize);
	if (result == NULL)
		error_msg_exit("Failed to reallocate memory in %s", __FILE__);

	return result;
}

// Free a specific Xanadu VM object based on its type.
// Releases memory and resources associated with the object.
//
// Parameters:
//   object - The object to free
static void free_object(Obj *object)
{
#ifdef DEBUG_LOG_GC
	printf("%p free type %d\n", (void *)object, object->type);
#endif

	// Free memory based on the object type
	switch (object->type) {
	case OBJ_BOUND_METHOD:
		FREE(ObjBoundMethod, object);
		break;
	case OBJ_INSTANCE: {
		ObjInstance *instance = (ObjInstance *)object;
		free_table(&instance->fields);
		FREE(ObjInstance, object);
		break;
	}
	case OBJ_CLASS: {
		ObjClass *klass = (ObjClass *)object;
		free_table(&klass->methods);
		FREE(ObjClass, object);
		break;
	}
	case OBJ_STRING: {
		ObjString *string = (ObjString *)object;
		FREE_ARRAY(char, string->chars, string->length + 1);
		FREE(ObjString, object);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction *function = (ObjFunction *)object;
		free_chunk(&function->chunk);
		FREE(ObjFunction, object);
		break;
	}
	case OBJ_NATIVE:
		FREE(ObjNative, object);
		break;
	case OBJ_CLOSURE: {
		ObjClosure *closure = (ObjClosure *)object;
		FREE_ARRAY(ObjUpvalue *, closure->upvalues,
			   closure->upvalueCount);
		FREE(ObjClosure, object);
		break;
	}
	case OBJ_UPVALUE:
		FREE(ObjUpvalue, object);
		break;
	}
}

// Free all objects currently in the Xanadu VM.
// This includes iterating through and freeing each object in the list.
//
// This function also frees the gray stack used for garbage collection.
void free_objects(void)
{
	Obj *object = vm.objects;
	while (object != NULL) {
		Obj *next = object->next;
		free_object(object);
		object = next;
	}

	free(vm.gray_stack);
}

// Mark a Xanadu object for garbage collection.
// This function ensures that the object is marked so that it is not
// collected during garbage collection.
//
// Parameters:
//   object - The object to mark
void mark_object(Obj *object)
{
	if (object == NULL)
		return;

	if (object->is_marked)
		return;

#ifdef DEBUG_LOG_GC
	printf("%p mark ", (void *)object);
	print_value(OBJ_VAL(object));
	printf("\n");
#endif

	object->is_marked = true;

	// Ensure there is enough space in the gray stack
	if (vm.gray_capacity < vm.gray_count + 1) {
		vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
		vm.gray_stack = (Obj **)realloc(
			vm.gray_stack, sizeof(Obj *) * vm.gray_capacity);
	}

	vm.gray_stack[vm.gray_count++] = object;

	if (vm.gray_stack == NULL)
		exit(1);
}

// Mark a Xanadu value for garbage collection if it is a heap object.
//
// Parameters:
//   value - The value to mark
void mark_value(Value value)
{
	if (IS_OBJ(value))
		mark_object(AS_OBJ(value));
}

// Mark all values in an array for garbage collection.
//
// Parameters:
//   array - The array of values to mark
static void mark_array(ValueArray *array)
{
	for (int i = 0; i < array->count; i++) {
		mark_value(array->values[i]);
	}
}

// Process an object for garbage collection.
// Marks the object and its references (if any) to ensure they are not
// collected prematurely.
//
// Parameters:
//   object - The object to process
static void blacken_object(Obj *object)
{
#ifdef DEBUG_LOG_GC
	printf("%p blacken ", (void *)object);
	print_value(OBJ_VAL(object));
	printf("\n");
#endif

	// Mark references contained within the object based on its type
	switch (object->type) {
	case OBJ_BOUND_METHOD: {
		ObjBoundMethod *bound = (ObjBoundMethod *)object;
		mark_value(bound->receiver);
		mark_object((Obj *)bound->method);
		break;
	}
	case OBJ_INSTANCE: {
		ObjInstance *instance = (ObjInstance *)object;
		mark_object((Obj *)instance->class_);
		mark_table(&instance->fields);
		break;
	}
	case OBJ_CLASS: {
		ObjClass *klass = (ObjClass *)object;
		mark_object((Obj *)klass->name);
		mark_table(&klass->methods);
		break;
	}
	case OBJ_CLOSURE: {
		ObjClosure *closure = (ObjClosure *)object;
		mark_object((Obj *)closure->function);
		for (int i = 0; i < closure->upvalueCount; i++) {
			mark_object((Obj *)closure->upvalues[i]);
		}
		break;
	}
	case OBJ_UPVALUE:
		mark_value(((ObjUpvalue *)object)->closed);
		break;
	case OBJ_FUNCTION: {
		ObjFunction *function = (ObjFunction *)object;
		mark_object((Obj *)function->name);
		mark_array(&function->chunk.constants);
		break;
	}
	case OBJ_NATIVE:
	case OBJ_STRING:
		// No additional marking required for native and string objects
		break;
	}
}

// Mark all global variables in the given table for garbage collection.
//
// Parameters:
//   table - The table containing global variables to mark
void mark_table(Table *table)
{
	for (int i = 0; i < table->capacity; i++) {
		Entry *entry = &table->entries[i];
		mark_object((Obj *)entry->key);
		mark_value(entry->value);
	}
}

// Mark the roots of the object graph for garbage collection.
// This includes the stack, frames, upvalues, global variables, and constants.
//
// This function sets the initial state for the garbage collector by marking
// all live objects.
static void mark_roots()
{
	// Mark values on the stack
	for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
		mark_value(*slot);
	}

	// Mark closures in the call frames
	for (int i = 0; i < vm.frameCount; i++) {
		mark_object((Obj *)vm.frames[i].closure);
	}

	// Mark open upvalues
	for (ObjUpvalue *upvalue = vm.openUpvalues; upvalue != NULL;
	     upvalue = upvalue->next) {
		mark_object((Obj *)upvalue);
	}

	// Mark global variables
	mark_table(&vm.globals);

	// Mark constants and literals
	mark_compiler_roots();
	mark_object((Obj *)vm.init_string);
}

// Trace and mark all reachable objects from the gray stack.
//
// This function processes all objects in the gray stack and marks their
// references, transitioning them to the black state.
static void trace_references()
{
	while (vm.gray_count > 0) {
		Obj *object = vm.gray_stack[--vm.gray_count];
		blacken_object(object);
	}
}

// Sweep through all objects and free those that are no longer marked.
//
// This function reclaims memory for objects that were not reachable during
// the garbage collection phase.
static void sweep()
{
	Obj *previous = NULL;
	Obj *object = vm.objects;
	while (object != NULL) {
		if (object->is_marked) {
			object->is_marked = false;
			previous = object;
			object = object->next;
		} else {
			Obj *unreached = object;
			object = object->next;
			if (previous != NULL) {
				previous->next = object;
			} else {
				vm.objects = object;
			}

			free_object(unreached);
			// Continue sweeping from the next object
		}
	}
}

// Perform garbage collection to reclaim unused memory.
// This involves marking roots, tracing references, and sweeping unreachable objects.
//
// This function updates the threshold for the next garbage collection cycle.
void collect_garbage(void)
{
#ifdef DEBUG_LOG_GC
	printf("-- gc begin\n");
	size_t before = vm.bytesAllocated;
#endif

	mark_roots(); // Mark all roots in the VM
	trace_references(); // Trace and mark all reachable objects
	table_remove_white(&vm.strings); // Remove and free unreferenced strings
	sweep(); // Free all unreachable objects

	// Set the threshold for the next garbage collection
	vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
	printf("-- gc end\n");
	printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
	       before - vm.bytesAllocated, before, vm.bytesAllocated,
	       vm.nextGC);
#endif
}
