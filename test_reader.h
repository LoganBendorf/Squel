#pragma once

#include "structs_and_macros.h"

#include <vector>

std::vector<struct test_container> init_read_test();
struct test read_test(struct test_container test, size_t index);
