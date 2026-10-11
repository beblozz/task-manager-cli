#include "TaskManager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

// ===== Вспомогательные функции (видны только в этом файле) =====

// Экранирование для JSON: кавычка внутри строки должна стать \"
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

// Экранирование для CSV: поле с запятой или кавычкой берём в кавычки,
// а кавычки внутри удваиваем (так делает Excel)
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

// Делит строку CSV по запятым, но запятые внутри кавычек не считает
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

// Простой разборщик JSON, который понимает только наш формат:
// массив объектов с полями id, title, deadline, done, priority, tags.
// Идёт по тексту символ за символом, pos — текущая позиция.
class JsonReader {
public:
    JsonReader(const std::string& text) : text(text), pos(0) {}

    void skipSpaces() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            pos++;
        }
    }

    // Посмотреть следующий символ, не сдвигаясь
    char peek() {
        skipSpaces();
        if (pos >= text.size()) {
            throw std::runtime_error("JSON: unexpected end of file");
        }
        return text[pos];
    }

    // Следующим обязан быть символ c, иначе файл испорчен
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
        // Task() по умолчанию имеет средний приоритет. Поэтому старые файлы
        // версии 1.0, где поля priority ещё не было, загружаются без ошибок.
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
            } else if (key == "priority") {
                task.setPriority(readInt());
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

// Проверка задачи, прочитанной из файла
static void validateLoadedTask(const Task& task, const std::string& where) {
    if (!task.getDeadline().empty() && !TaskManager::isValidDate(task.getDeadline())) {
        throw std::runtime_error(where + ": wrong date in task " + std::to_string(task.getId()));
    }
    if (!TaskManager::isValidPriority(task.getPriority())) {
        throw std::runtime_error(where + ": wrong priority in task " + std::to_string(task.getId()));
    }
}

// Два одинаковых id сломали бы удаление и редактирование
static void checkUniqueIds(const std::vector<Task>& list, const std::string& where) {
    for (size_t i = 0; i < list.size(); i++) {
        for (size_t j = i + 1; j < list.size(); j++) {
            if (list[i].getId() == list[j].getId()) {
                throw std::runtime_error(where + ": duplicate id " + std::to_string(list[i].getId()));
            }
        }
    }
}

// ===== TaskManager =====

TaskManager::TaskManager() : nextId(1) {}

int TaskManager::addTask(const std::string& title, const std::string& deadline,
                         const std::vector<std::string>& tags, int priority) {
    if (title.empty()) {
        throw std::invalid_argument("Title cannot be empty");
    }
    if (!deadline.empty() && !isValidDate(deadline)) {
        throw std::invalid_argument("Wrong date format, use YYYY-MM-DD");
    }
    if (!isValidPriority(priority)) {
        throw std::invalid_argument("Priority must be 1 (low), 2 (medium) or 3 (high)");
    }
    Task task(nextId, title, deadline, tags, priority);
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

void TaskManager::editTask(int id,
                           const std::optional<std::string>& title,
                           const std::optional<std::string>& deadline,
                           const std::optional<int>& priority) {
    int index = findIndex(id);

    // Шаг 1: проверяем всё, ничего не меняя
    if (title && title->empty()) {
        throw std::invalid_argument("Title cannot be empty");
    }
    if (deadline && !deadline->empty() && !isValidDate(*deadline)) {
        throw std::invalid_argument("Wrong date format, use YYYY-MM-DD");
    }
    if (priority && !isValidPriority(*priority)) {
        throw std::invalid_argument("Priority must be 1 (low), 2 (medium) or 3 (high)");
    }

    // Шаг 2: все данные верные — меняем
    if (title) {
        tasks[index].setTitle(*title);
    }
    if (deadline) {
        tasks[index].setDeadline(*deadline);
    }
    if (priority) {
        tasks[index].setPriority(*priority);
    }
}

Task TaskManager::getTask(int id) const {
    return tasks[findIndex(id)];
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

// Сравнение по дедлайну: задачи без дедлайна уходят в конец
static bool deadlineLess(const Task& a, const Task& b) {
    if (a.getDeadline().empty()) {
        return false;
    }
    if (b.getDeadline().empty()) {
        return true;
    }
    return a.getDeadline() < b.getDeadline();
}

std::vector<Task> TaskManager::getSortedByDeadline() const {
    std::vector<Task> result = tasks;
    // stable_sort сохраняет исходный порядок у задач с одинаковым дедлайном
    std::stable_sort(result.begin(), result.end(), deadlineLess);
    return result;
}

std::vector<Task> TaskManager::getSortedByPriority() const {
    std::vector<Task> result = tasks;
    std::stable_sort(result.begin(), result.end(), [](const Task& a, const Task& b) {
        if (a.getPriority() != b.getPriority()) {
            return a.getPriority() > b.getPriority();
        }
        return deadlineLess(a, b);
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
        out << "    \"priority\": " << task.getPriority() << ",\n";
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

    // Сначала читаем во временный вектор: если файл испорчен,
    // текущие задачи не пропадут
    std::vector<Task> loaded = reader.readTasks();
    for (const Task& task : loaded) {
        validateLoadedTask(task, "JSON");
    }
    checkUniqueIds(loaded, "JSON");
    tasks = loaded;
    updateNextId();
}

void TaskManager::saveToCsv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    // priority — последний столбец, чтобы старые CSV (без него) тоже читались
    file << "id,title,deadline,done,tags,priority\n";
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
             << escapeCsv(tagsLine) << ","
             << task.getPriority() << "\n";
    }
}

void TaskManager::loadFromCsv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::vector<Task> loaded;
    std::string line;
    int lineNumber = 1;
    std::getline(file, line);  // пропускаем заголовок
    while (std::getline(file, line)) {
        lineNumber++;
        // В файлах из Windows строка может кончаться на \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        std::string where = "CSV line " + std::to_string(lineNumber);
        std::vector<std::string> fields = splitCsvLine(line);
        // 5 столбцов — файл версии 1.0, 6 — с приоритетом
        if (fields.size() != 5 && fields.size() != 6) {
            throw std::runtime_error(where + ": wrong number of fields");
        }
        Task task;
        try {
            task.setId(std::stoi(fields[0]));
            if (fields.size() == 6) {
                task.setPriority(std::stoi(fields[5]));
            }
        } catch (const std::exception&) {
            throw std::runtime_error(where + ": wrong number in id or priority");
        }
        task.setTitle(fields[1]);
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
        validateLoadedTask(task, where);
        loaded.push_back(task);
    }
    checkUniqueIds(loaded, "CSV");
    tasks = loaded;
    updateNextId();
}

std::string TaskManager::toLower(const std::string& s) {
    std::string result;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = s[i];
        // Латиница: 1 байт, справляется std::tolower
        if (c < 128) {
            result += static_cast<char>(std::tolower(c));
            continue;
        }
        // Кириллица в UTF-8: 2 байта. Заглавные А-П (D0 90..9F) -> а-п (D0 B0..BF),
        // Р-Я (D0 A0..AF) -> р-я (D1 80..8F), Ё (D0 81) -> ё (D1 91)
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
    // Високосный: делится на 4, но не на 100, или делится на 400
    bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (leap) {
        daysInMonth[1] = 29;
    }
    return day <= daysInMonth[month - 1];
}

bool TaskManager::isValidPriority(int priority) {
    return priority >= Task::PRIORITY_LOW && priority <= Task::PRIORITY_HIGH;
}

int TaskManager::findIndex(int id) const {
    for (size_t i = 0; i < tasks.size(); i++) {
        if (tasks[i].getId() == id) {
            return static_cast<int>(i);
        }
    }
    throw std::out_of_range("Task with id " + std::to_string(id) + " not found");
}

// После загрузки из файла следующий id = максимальный + 1
void TaskManager::updateNextId() {
    nextId = 1;
    for (const Task& task : tasks) {
        if (task.getId() >= nextId) {
            nextId = task.getId() + 1;
        }
    }
}
