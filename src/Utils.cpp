#include "Utils.h"

#include <ctime>
#include <sstream>

std::string getToday() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", local);
    return std::string(buffer);
}

std::vector<std::string> parseTags(const std::string& line) {
    std::vector<std::string> tags;
    std::stringstream ss(line);
    std::string tag;
    // оператор >> читает слова, пропуская пробелы
    while (ss >> tag) {
        if (tag[0] == '#') {
            tag = tag.substr(1);
        }
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }
    return tags;
}
