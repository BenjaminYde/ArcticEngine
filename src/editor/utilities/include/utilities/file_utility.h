//
// Created by Benjamin on 22/11/2022.
//

#ifndef ARCTIC_FILE_UTILITY_H
#define ARCTIC_FILE_UTILITY_H

#include <iostream>
#include <vector>

class FileUtility
{
public:
    static bool ReadBinaryFile(const std::string &path, std::vector<char>&buffer);

#endif //ARCTIC_FILE_UTILITY_H
};