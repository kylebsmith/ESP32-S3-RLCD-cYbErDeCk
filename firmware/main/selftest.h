#pragma once
#include <stdbool.h>
/* Returns true when a test stage ran and normal startup should be skipped. */
bool selftest_run(void);
void selftest_reset(void);
