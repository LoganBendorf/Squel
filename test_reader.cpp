

#include "pch.h"

#include <filesystem>
#include <fstream>
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

    // Sort tests in folder
    for (auto& test : tests) {
        std::sort(test.test_paths.begin(), test.test_paths.end());
    }

    if (tests.size() == 0) {
        printf("NO TESTS\n");
        exit(1);
    }

    std::cout << "PRINTING TEST NAMES ------------\n";
    for (const auto& test : tests) {
        for (const auto& test_path : test.test_paths) {
            std::cout << " " << test_path << std::endl;
        }
    }
    std::cout << "DONE ---------------------------\n\n";

    return tests;
}

std::string read_test(struct test test, size_t index) {
    
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