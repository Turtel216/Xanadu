// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by a MIT
// license that can be found in the LICENSE file.

#ifndef xanadu_compiler_h
#define xanadu_compiler_h

#include "chunk.h"
#include "object.h"

// Compile source string
ObjFunction *compile(const char *source);

#endif
