// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#include <stdlib.h>

#include "memory.h"
#include "error.h"
#include "object.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

// Realocate given type
void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
	if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
		collect_garbage();
#endif
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

// Free Xanadu VM object
static void free_object(Obj *object)
{
#ifdef DEBUG_LOG_GC
	printf("%p free type %d\n", (void *)object, object->type);
#endif

	// Check for object type and free accordingly
	switch (object->type) {
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

// Free all objects from Xanadu VM
void free_objects(void)
{
	Obj *object = vm.objects;
	while (object != NULL) {
		Obj *next = object->next;
		free_object(object);
		object = next;
	}
}

// Free unused xanadu variables
void collect_garbage()
{
#ifdef DEBUG_LOG_GC
	printf("-- gc begin\n");
#endif

#ifdef DEBUG_LOG_GC
	printf("-- gc end\n");
#endif
}
