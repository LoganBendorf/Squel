

#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

extern std::vector<std::string> errors;

static int max_tests = -1;
static int current_test_num = 0;

std::vector<std::string> test_paths;

std::string read_test() {
    const std::string path = "tests";
    if (max_tests == -1) {
        max_tests = 0;
        for (const auto & entry: std::filesystem::directory_iterator(path)) {
            test_paths.push_back(entry.path());
            max_tests++;
        }
        
        if (max_tests == 0) {
            printf("NO TESTS\n");
            exit(1);
        }

        for (int i = 0; i < test_paths.size(); i++) {
            std::cout << test_paths[i] << std::endl;
        }
    }

    if (current_test_num == max_tests) {
        return "NO MORE TESTS!!!!";
    }

    std::ifstream file;
    file.open(test_paths[current_test_num]);

    if (!file.is_open()) {
        errors.push_back("Could not open file: " + test_paths[current_test_num]);
        exit(1);
    }

    std::string file_contents((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>()); // Read entire file content
    file.close(); // Added missing semicolon

    current_test_num++;

    return file_contents;

}