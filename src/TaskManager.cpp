#include "TaskManager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

static std::string escapeJson(const std::string& s) {
    std::string result;
    for (char c : s) {
        if (c == '"') {
            result += "\\\"";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c == '\n') {
            result += "\\n";
        } else {
            result += c;
        }
    }
    return result;
}

static std::string escapeCsv(const std::string& s) {
    if (s.find(',') == std::string::npos && s.find('"') == std::string::npos) {
        return s;
    }
    std::string result = "\"";
    for (char c : s) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

static std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    i++;
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
    }
    if (inQuotes) {
        throw std::runtime_error("CSV: unclosed quote");
    }
    fields.push_back(current);
    return fields;
}

class JsonReader {
public:
    JsonReader(const std::string& text) : text(text), pos(0) {}

    void skipSpaces() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            pos++;
        }
    }

    char peek() {
        skipSpaces();
        if (pos >= text.size()) {
            throw std::runtime_error("JSON: unexpected end of file");
        }
        return text[pos];
    }

    void expect(char c) {
        if (peek() != c) {
            throw std::runtime_error(std::string("JSON: expected '") + c + "' at position " + std::to_string(pos));
        }
        pos++;
    }

    std::string readString() {
        expect('"');
        std::string result;
        while (pos < text.size() && text[pos] != '"') {
            if (text[pos] == '\\') {
                pos++;
                if (pos >= text.size()) {
                    break;
                }
                if (text[pos] == 'n') {
                    result += '\n';
                } else {
                    result += text[pos];
                }
            } else {
                result += text[pos];
            }
            pos++;
        }
        expect('"');
        return result;
    }

    int readInt() {
        skipSpaces();
        size_t start = pos;
        if (pos < text.size() && text[pos] == '-') {
            pos++;
        }
        while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            pos++;
        }
        if (start == pos) {
            throw std::runtime_error("JSON: expected number at position " + std::to_string(pos));
        }
        return std::stoi(text.substr(start, pos - start));
    }

    bool readBool() {
        skipSpaces();
        if (text.compare(pos, 4, "true") == 0) {
            pos += 4;
            return true;
        }
        if (text.compare(pos, 5, "false") == 0) {
            pos += 5;
            return false;
        }
        throw std::runtime_error("JSON: expected true or false at position " + std::to_string(pos));
    }

    std::vector<std::string> readStringArray() {
        std::vector<std::string> result;
        expect('[');
        if (peek() == ']') {
            pos++;
            return result;
        }
        while (true) {
            result.push_back(readString());
            if (peek() == ',') {
                pos++;
            } else {
                break;
            }
        }
        expect(']');
        return result;
    }

    Task readTask() {
        Task task;
        expect('{');
        if (peek() == '}') {
            pos++;
            return task;
        }
        while (true) {
            std::string key = readString();
            expect(':');
            if (key == "id") {
                task.setId(readInt());
            } else if (key == "title") {
                task.setTitle(readString());
            } else if (key == "deadline") {
                task.setDeadline(readString());
            } else if (key == "done") {
                task.setDone(readBool());
            } else if (key == "tags") {
                task.setTags(readStringArray());
            } else {
                throw std::runtime_error("JSON: unknown key '" + key + "'");
            }
            if (peek() == ',') {
                pos++;
            } else {
                break;
            }
        }
        expect('}');
        return task;
    }

    std::vector<Task> readTasks() {
        std::vector<Task> result;
        expect('[');
        if (peek() == ']') {
            pos++;
            return result;
        }
        while (true) {
            result.push_back(readTask());
            if (peek() == ',') {
                pos++;
            } else {
                break;
            }
        }
        expect(']');
        return result;
    }

private:
    std::string text;
    size_t pos;
};

TaskManager::TaskManager() : nextId(1) {}

int TaskManager::addTask(const std::string& title, const std::string& deadline, const std::vector<std::string>& tags) {
    if (title.empty()) {
        throw std::invalid_argument("Title cannot be empty");
    }
    if (!deadline.empty() && !isValidDate(deadline)) {
        throw std::invalid_argument("Wrong date format, use YYYY-MM-DD");
    }
    Task task(nextId, title, deadline, tags);
    tasks.push_back(task);
    nextId++;
    return task.getId();
}

void TaskManager::removeTask(int id) {
    int index = findIndex(id);
    tasks.erase(tasks.begin() + index);
}

void TaskManager::markDone(int id) {
    int index = findIndex(id);
    tasks[index].setDone(true);
}

std::vector<Task> TaskManager::getAll() const {
    return tasks;
}

std::vector<Task> TaskManager::searchByText(const std::string& text) const {
    std::vector<Task> result;
    std::string query = toLower(text);
    for (const Task& task : tasks) {
        if (toLower(task.getTitle()).find(query) != std::string::npos) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<Task> TaskManager::searchByTag(const std::string& tag) const {
    std::vector<Task> result;
    for (const Task& task : tasks) {
        if (task.hasTag(tag)) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<Task> TaskManager::getSortedByDeadline() const {
    std::vector<Task> result = tasks;
    std::sort(result.begin(), result.end(), [](const Task& a, const Task& b) {
        if (a.getDeadline().empty()) {
            return false;
        }
        if (b.getDeadline().empty()) {
            return true;
        }
        return a.getDeadline() < b.getDeadline();
    });
    return result;
}

std::vector<Task> TaskManager::getOverdue(const std::string& today) const {
    std::vector<Task> result;
    for (const Task& task : tasks) {
        if (task.isOverdue(today)) {
            result.push_back(task);
        }
    }
    return result;
}

std::string TaskManager::toJson(const std::vector<Task>& list) {
    std::stringstream out;
    out << "[\n";
    for (size_t i = 0; i < list.size(); i++) {
        const Task& task = list[i];
        out << "  {\n";
        out << "    \"id\": " << task.getId() << ",\n";
        out << "    \"title\": \"" << escapeJson(task.getTitle()) << "\",\n";
        out << "    \"deadline\": \"" << escapeJson(task.getDeadline()) << "\",\n";
        out << "    \"done\": " << (task.isDone() ? "true" : "false") << ",\n";
        out << "    \"tags\": [";
        std::vector<std::string> tags = task.getTags();
        for (size_t j = 0; j < tags.size(); j++) {
            out << "\"" << escapeJson(tags[j]) << "\"";
            if (j + 1 < tags.size()) {
                out << ", ";
            }
        }
        out << "]\n";
        out << "  }";
        if (i + 1 < list.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "]\n";
    return out.str();
}

void TaskManager::saveToJson(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    file << toJson(tasks);
}

void TaskManager::loadFromJson(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    JsonReader reader(buffer.str());
    std::vector<Task> loaded = reader.readTasks();
    for (const Task& task : loaded) {
        if (!task.getDeadline().empty() && !isValidDate(task.getDeadline())) {
            throw std::runtime_error("JSON: wrong date in task " + std::to_string(task.getId()));
        }
    }
    tasks = loaded;
    updateNextId();
}

void TaskManager::saveToCsv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    file << "id,title,deadline,done,tags\n";
    for (const Task& task : tasks) {
        std::string tagsLine;
        std::vector<std::string> tags = task.getTags();
        for (size_t i = 0; i < tags.size(); i++) {
            tagsLine += tags[i];
            if (i + 1 < tags.size()) {
                tagsLine += ";";
            }
        }
        file << task.getId() << ","
             << escapeCsv(task.getTitle()) << ","
             << task.getDeadline() << ","
             << (task.isDone() ? "1" : "0") << ","
             << escapeCsv(tagsLine) << "\n";
    }
}

void TaskManager::loadFromCsv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::vector<Task> loaded;
    std::string line;
    int lineNumber = 0;
    std::getline(file, line);
    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> fields = splitCsvLine(line);
        if (fields.size() != 5) {
            throw std::runtime_error("CSV: wrong number of fields in line " + std::to_string(lineNumber + 1));
        }
        Task task;
        try {
            task.setId(std::stoi(fields[0]));
        } catch (const std::exception&) {
            throw std::runtime_error("CSV: wrong id in line " + std::to_string(lineNumber + 1));
        }
        task.setTitle(fields[1]);
        if (!fields[2].empty() && !isValidDate(fields[2])) {
            throw std::runtime_error("CSV: wrong date in line " + std::to_string(lineNumber + 1));
        }
        task.setDeadline(fields[2]);
        task.setDone(fields[3] == "1");
        std::vector<std::string> tags;
        std::stringstream ss(fields[4]);
        std::string tag;
        while (std::getline(ss, tag, ';')) {
            if (!tag.empty()) {
                tags.push_back(tag);
            }
        }
        task.setTags(tags);
        loaded.push_back(task);
    }
    tasks = loaded;
    updateNextId();
}

std::string TaskManager::toLower(const std::string& s) {
    std::string result;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = s[i];
        if (c < 128) {
            result += static_cast<char>(std::tolower(c));
            continue;
        }
        if (i + 1 < s.size()) {
            unsigned char next = s[i + 1];
            if (c == 0xD0 && next >= 0x90 && next <= 0x9F) {
                result += static_cast<char>(0xD0);
                result += static_cast<char>(next + 0x20);
                i++;
                continue;
            }
            if (c == 0xD0 && next >= 0xA0 && next <= 0xAF) {
                result += static_cast<char>(0xD1);
                result += static_cast<char>(next - 0x20);
                i++;
                continue;
            }
            if (c == 0xD0 && next == 0x81) {
                result += static_cast<char>(0xD1);
                result += static_cast<char>(0x91);
                i++;
                continue;
            }
        }
        result += static_cast<char>(c);
    }
    return result;
}

bool TaskManager::isValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) {
            return false;
        }
    }
    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    if (month < 1 || month > 12 || day < 1) {
        return false;
    }
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (leap) {
        daysInMonth[1] = 29;
    }
    return day <= daysInMonth[month - 1];
}

int TaskManager::findIndex(int id) const {
    for (size_t i = 0; i < tasks.size(); i++) {
        if (tasks[i].getId() == id) {
            return static_cast<int>(i);
        }
    }
    throw std::out_of_range("Task with id " + std::to_string(id) + " not found");
}

void TaskManager::updateNextId() {
    nextId = 1;
    for (const Task& task : tasks) {
        if (task.getId() >= nextId) {
            nextId = task.getId() + 1;
        }
    }
}
