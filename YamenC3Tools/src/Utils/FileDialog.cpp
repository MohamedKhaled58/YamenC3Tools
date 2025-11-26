#include "FileDialog.h"
#include <Commdlg.h>

bool FileDialog::OpenFile(const char* filter, std::string& outPath, HWND owner) {
    char filename[MAX_PATH] = { 0 };

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        outPath = filename;
        return true;
    }

    return false;
}

bool FileDialog::SaveFile(const char* filter, const char* defaultExt, std::string& outPath, HWND owner) {
    char filename[MAX_PATH] = { 0 };

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) {
        outPath = filename;
        return true;
    }

    return false;
}

bool FileDialog::OpenMultipleFiles(const char* filter, std::vector<std::string>& outPaths, HWND owner) {
    char filenames[4096] = { 0 };

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filenames;
    ofn.nMaxFile = sizeof(filenames);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        std::string directory = filenames;
        char* file = filenames + directory.length() + 1;

        if (*file == '\0') {
            // Single file selected
            outPaths.push_back(directory);
        }
        else {
            // Multiple files selected
            while (*file) {
                outPaths.push_back(directory + "\\" + file);
                file += strlen(file) + 1;
            }
        }
        return true;
    }

    return false;
}