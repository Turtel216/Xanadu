// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#ifndef xanadu_debug_h
#define xanadu_debug_h

#include "chunk.h"

void disassemble_chunk(Chunk *chunk, const char *name);
int disassemble_instruction(Chunk *chunk, int offset);

#endif
