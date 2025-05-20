//
// Created by Pavel on 19.05.2025.
//
#pragma once

#include <fstream>

namespace util {

inline bool FilenameBasicSyntaxIsValid(const std::string& filename) {
    // Check for empty filename
    if (filename.empty()) {
        return false;
    }

    // Check filename length
    if (filename.size() > 255) {
        return false;
    }

    // Check for invalid characters
    const std::string invalidChars = "\\/:*?\"<>|";
    return std::none_of(filename.begin(), filename.end(),
        [&invalidChars](char c) {
            return invalidChars.find(c) != std::string::npos;
        }
    );
}

// Set input = false to open file in output mode
inline void OpenFile(std::fstream& stream, const std::string& file_path, bool read_mode = true) {
    if(!FilenameBasicSyntaxIsValid(file_path)) {
        throw std::invalid_argument("invalid file path: [" + file_path + "]");
    }

    if(read_mode) {
        stream.open(file_path, std::ios_base::binary | std::ios_base::in);
    } else {
        stream.open(file_path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    }
    if(!stream) {
        throw std::runtime_error("unable to open file: [" + file_path + "]");
    }
}

template<typename... Streams>
void CloseFiles(Streams&... streams) {
    // Read stream failbit will be set, so check badbit only
    if((streams.bad() && ...)) {
        throw std::runtime_error("iostream error occured at closing streams");
    }
    // Close each stream using fold expression
    (streams.close(), ...);
}

}