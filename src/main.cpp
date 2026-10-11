#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "TaskManager.h"
#include "Utils.h"

const std::string DEFAULT_FILE = "tasks.json";
const std::string DEFAULT_CSV = "tasks.csv";

// Своё исключение для конца ввода (Ctrl+Z / Ctrl+D).
// Раньше мы узнавали его по тексту сообщения — это хрупко:
// поменяешь текст, и проверка сломается. Отдельный тип надёжнее.
class InputClosedError : public std::runtime_error {
public:
    InputClosedError() : std::runtime_error("Input closed") {}
};

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputClosedError();
    }
    return line;
}

int parseInt(const std::string& line) {
    try {
        size_t pos = 0;
        int value = std::stoi(line, &pos);
        // "12abc" — не число, хотя stoi прочитал бы 12
        if (pos != line.size()) {
            throw std::invalid_argument("extra symbols");
        }
        return value;
    } catch (const std::exception&) {
        throw std::invalid_argument("Please enter a number");
    }
}

int readInt(const std::string& prompt) {
    return parseInt(readLine(prompt));
}

// Пустой ввод = std::nullopt ("не менять" или "по умолчанию")
std::optional<int> readOptionalInt(const std::string& prompt) {
    std::string line = readLine(prompt);
    if (line.empty()) {
        return std::nullopt;
    }
    return parseInt(line);
}

std::string readFilename(const std::string& defaultName) {
    std::string filename = readLine("File name (empty = " + defaultName + "): ");
    return filename.empty() ? defaultName : filename;
}

void printTasks(const std::vector<Task>& tasks) {
    if (tasks.empty()) {
        std::cout << "No tasks.\n";
        return;
    }
    for (const Task& task : tasks) {
        std::cout << task.toString() << "\n";
    }
}

void printMenu() {
    std::cout << "\n===== Task Manager =====\n";
    std::cout << "1. Add task\n";
    std::cout << "2. Show all tasks\n";
    std::cout << "3. Edit task\n";
    std::cout << "4. Remove task\n";
    std::cout << "5. Mark task as done\n";
    std::cout << "6. Search by text\n";
    std::cout << "7. Search by tag\n";
    std::cout << "8. Sort by deadline\n";
    std::cout << "9. Sort by priority\n";
    std::cout << "10. Show overdue tasks\n";
    std::cout << "11. Save to JSON\n";
    std::cout << "12. Load from JSON\n";
    std::cout << "13. Export to CSV\n";
    std::cout << "14. Import from CSV\n";
    std::cout << "0. Exit\n";
}

void addTask(TaskManager& manager) {
    std::string title = readLine("Title: ");
    std::string deadline = readLine("Deadline (YYYY-MM-DD, empty to skip): ");
    std::string tagsLine = readLine("Tags (separated by space): ");
    std::optional<int> priority = readOptionalInt("Priority (1 low, 2 medium, 3 high, empty = 2): ");
    int id = manager.addTask(title, deadline, parseTags(tagsLine),
                             priority.value_or(Task::PRIORITY_MEDIUM));
    std::cout << "Task added with id " << id << "\n";
}

void editTask(TaskManager& manager) {
    int id = readInt("Task id: ");
    // getTask бросит исключение, если задачи нет — узнаем об этом сразу,
    // а не после того, как человек введёт все поля
    std::cout << "Now: " << manager.getTask(id).toString() << "\n";
    std::cout << "Press Enter to keep the current value\n";

    std::optional<std::string> title;
    std::string line = readLine("New title: ");
    if (!line.empty()) {
        title = line;
    }

    std::optional<std::string> deadline;
    line = readLine("New deadline (YYYY-MM-DD, '-' to remove): ");
    if (line == "-") {
        deadline = "";
    } else if (!line.empty()) {
        deadline = line;
    }

    std::optional<int> priority = readOptionalInt("New priority (1 low, 2 medium, 3 high): ");

    manager.editTask(id, title, deadline, priority);
    std::cout << "Task updated: " << manager.getTask(id).toString() << "\n";
}

int main() {
    TaskManager manager;
    bool autoSave = true;

    std::ifstream check(DEFAULT_FILE);
    if (check.good()) {
        check.close();
        try {
            manager.loadFromJson(DEFAULT_FILE);
            std::cout << "Loaded " << manager.getAll().size() << " tasks from " << DEFAULT_FILE << "\n";
        } catch (const std::exception& e) {
            // Не затираем испорченный файл при выходе
            std::cout << "Could not load " << DEFAULT_FILE << ": " << e.what() << "\n";
            std::cout << "Auto save is off, fix the file or save manually\n";
            autoSave = false;
        }
    }

    bool running = true;
    while (running) {
        printMenu();
        try {
            int choice = readInt("> ");
            switch (choice) {
            case 1:
                addTask(manager);
                break;
            case 2:
                printTasks(manager.getAll());
                break;
            case 3:
                editTask(manager);
                break;
            case 4:
                manager.removeTask(readInt("Task id: "));
                std::cout << "Task removed\n";
                break;
            case 5:
                manager.markDone(readInt("Task id: "));
                std::cout << "Task marked as done\n";
                break;
            case 6:
                printTasks(manager.searchByText(readLine("Text: ")));
                break;
            case 7: {
                std::vector<std::string> tags = parseTags(readLine("Tag: "));
                if (tags.empty()) {
                    throw std::invalid_argument("Tag cannot be empty");
                }
                printTasks(manager.searchByTag(tags[0]));
                break;
            }
            case 8:
                printTasks(manager.getSortedByDeadline());
                break;
            case 9:
                printTasks(manager.getSortedByPriority());
                break;
            case 10:
                std::cout << "Today: " << getToday() << "\n";
                printTasks(manager.getOverdue(getToday()));
                break;
            case 11: {
                std::string filename = readFilename(DEFAULT_FILE);
                manager.saveToJson(filename);
                std::cout << "Saved to " << filename << "\n";
                break;
            }
            case 12:
                manager.loadFromJson(readFilename(DEFAULT_FILE));
                std::cout << "Loaded " << manager.getAll().size() << " tasks\n";
                break;
            case 13: {
                std::string filename = readFilename(DEFAULT_CSV);
                manager.saveToCsv(filename);
                std::cout << "Exported to " << filename << "\n";
                break;
            }
            case 14:
                manager.loadFromCsv(readFilename(DEFAULT_CSV));
                std::cout << "Imported " << manager.getAll().size() << " tasks\n";
                break;
            case 0:
                running = false;
                break;
            default:
                std::cout << "Unknown command\n";
            }
        } catch (const InputClosedError&) {
            // Порядок catch важен: сначала конкретный тип, потом общий
            running = false;
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }

    if (!autoSave) {
        std::cout << "Bye!\n";
        return 0;
    }
    try {
        manager.saveToJson(DEFAULT_FILE);
        std::cout << "Tasks saved to " << DEFAULT_FILE << ". Bye!\n";
    } catch (const std::exception& e) {
        std::cout << "Could not save tasks: " << e.what() << "\n";
    }
    return 0;
}
