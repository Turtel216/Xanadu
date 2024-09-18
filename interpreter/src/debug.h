// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by a MIT
// license that can be found in the LICENSE file.

#ifndef xanadu_debug_h
#define xanadu_debug_h

#include "chunk.h"

void disassemble_chunk(Chunk *chunk, const char *name);
int disassemble_instruction(Chunk *chunk, int offset);

#endif
