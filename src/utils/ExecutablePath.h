#pragma once

#include <filesystem>

// The program's own file (voxima.exe, say), wherever it was run from; empty if it cannot be found. What is
// installed with it is found from there (see Producer.cpp).
std::filesystem::path executable_path();
