module;

#include <vector>

export module test_reader;

import structs;

export {

std::vector<test_container> init_read_test();
test read_test(test_container test, size_t index);

}
