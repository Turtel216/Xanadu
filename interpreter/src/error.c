// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by a MIT
// license that can be found in the LICENSE file.

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

void error_msg_exit(const char *fmt, ...)
{
	char msg_buff[80];

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg_buff, sizeof(msg_buff), fmt, ap);

	printf("%s", msg_buff);
	exit(1);
}
