#ifndef xanadu_compiler_h
#define xanadu_compiler_h

#include "chunk.h"
#include "object.h"

ObjFunction *compile(const char *source, Chunk *chunk);

#endif
