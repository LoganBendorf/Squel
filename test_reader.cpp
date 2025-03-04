

#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

#include "structs_and_macros.h"

extern std::vector<std::string> errors;

std::vector<struct test> init_read_test() {
    const std::string path = "tests";
    std::vector<struct test> tests;

    for (const auto & entry: std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            struct test test;
            test.folder_name = entry.path().lexically_relative("tests");
            for (const auto & sub_folder_entry: std::filesystem::directory_iterator(entry.path())){
                test.test_paths.push_back(sub_folder_entry.path().lexically_relative("tests"));
                test.max_tests++;
            }
            tests.push_back(test);
        } else {
            std::cout << "tests should be in a folder" << std::endl;
        }
    }

    for (int i = 0; i < tests.size(); i++) {
        std::sort(tests[i].test_paths.begin(), tests[i].test_paths.end());

    }
    
    if (tests.size() == 0) {
        printf("NO TESTS\n");
        exit(1);
    }

    std::cout << "PRINTING TEST NAMES ------------\n";
    for (int i = 0; i < tests.size(); i++) {
        for (int j = 0; j < tests[i].test_paths.size(); j++) {
            std::cout << " " << tests[i].test_paths[j] << std::endl;
        }
    }
    std::cout << "DONE ---------------------------\n\n";

    return tests;
}

std::string read_test(struct test test, int index) {
    
    if (test.current_test_num == test.max_tests) {
        return "NO MORE TESTS!!!!";
    }

    std::ifstream file;
    file.open("tests/" + test.test_paths[index]);

    if (!file.is_open()) {
        errors.push_back("Could not open file: " + test.test_paths[index]);
        exit(1);
    }

    std::string file_contents((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>()); // Read entire file content
    file.close();

    test.current_test_num++;

    return file_contents;

}