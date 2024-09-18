// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by a MIT
// license that can be found in the LICENSE file.
//
// This file contains the implementation the parser for Xanadu. It defines parsing rules
// and precedence, manages compiler state, and handles expressions, variables, and control flow structures.

#ifndef xanadu_compiler_h
#define xanadu_compiler_h

#include "chunk.h"
#include "object.h"

// Compile the source string into a function.
ObjFunction *compile(const char *source);

// Mark values in the compiler's scope as roots (for garbage collection).
void mark_compiler_roots();

#endif
