#include "utilities/file_utility.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

bool FileUtility::ReadBinaryFile(const std::string& path, std::vector<char>& buffer)
{
    // return when path does not exist
    fs::path fsPath(path);
    bool pathExists = fs::exists(fsPath);
    if(!pathExists)
        return false;

    // try read file
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        return false;

    // create buffer
    size_t fileSize = (size_t) file.tellg();
    buffer = std::vector<char>(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    // finish reading
    file.close();

    return true;
}