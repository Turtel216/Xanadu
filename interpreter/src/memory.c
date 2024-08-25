// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#include <stdlib.h>

#include "memory.h"
#include "error.h"
#include "object.h"
#include "vm.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
	if (newSize == 0) {
		free(pointer);
		return NULL;
	}

	void *result = realloc(pointer, newSize);
	if (result == NULL)
		error_msg_exit("Failed to reallocate memory in %s", __FILE__);

	return result;
}

static void free_object(Obj *object)
{
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

void free_objects()
{
	Obj *object = vm.objects;
	while (object != NULL) {
		Obj *next = object->next;
		free_object(object);
		object = next;
	}
}
