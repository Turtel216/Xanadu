#include <stdlib.h>

#include "memory.h"
#include "error.h"

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
