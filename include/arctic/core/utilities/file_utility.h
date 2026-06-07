#pragma once

#include <iostream>
#include <vector>

class FileUtility {
  public:
    static bool ReadBinaryFile(const std::string& path, std::vector<char>& buffer);
};