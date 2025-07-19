#pragma once

#include "allocator_aliases.h"

class table_object;

// Save
bool save_tables(const avec<SP<table_object>>& tables, const std::string& out_path = "saved/output.txt");

// Read
bool read_saved_files(avec<SP<table_object>>& tables, const std::filesystem::path& in_path = "saved/output.txt");