#pragma once
#include <string>
#include <vector>
#include <Windows.h>

class FileDialog {
public:
    static bool OpenFile(const char* filter, std::string& outPath, HWND owner = nullptr);
    static bool SaveFile(const char* filter, const char* defaultExt, std::string& outPath, HWND owner = nullptr);
    static bool OpenMultipleFiles(const char* filter, std::vector<std::string>& outPaths, HWND owner = nullptr);
};