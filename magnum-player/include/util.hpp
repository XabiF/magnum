
#pragma once
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <iostream>

// Enable logging
#define LOG_ENABLE

#ifdef LOG_ENABLE

#define LOG_FMT(...) std::cout << "[" << __FUNCTION__ << "] " <<  __VA_ARGS__ << std::endl

#else

#define LOG_FMT

#endif

inline bool StringStartsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

inline std::string GetFileName(const std::string &file) {
    return file.substr(0, file.find_last_of("."));
}

inline std::string GetFileExtension(const std::string &file) {
    return file.substr(file.find_last_of(".") + 1);
}

inline bool ExistsFile(const std::string &path) {
    struct stat st;
    if(stat(path.c_str(), &st) == 0) {
        return st.st_mode & S_IFREG;
    }

    return false;
}

inline std::string PathJoin(const std::string &path_1, const std::string &path_2) {
    #ifdef _WIN32
        return path_1 + "\\" + path_2;
    #else
        return path_1 + "/" + path_2;
    #endif
}
