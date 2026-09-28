#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TaskManager.h"

const std::string DEFAULT_FILE = "tasks.json";

std::string getToday() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", local);
    return std::string(buffer);
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw std::runtime_error("Input closed");
    }
    return line;
}

int readInt(const std::string& prompt) {
    std::string line = readLine(prompt);
    try {
        size_t pos = 0;
        int value = std::stoi(line, &pos);
        if (pos != line.size()) {
            throw std::invalid_argument("extra symbols");
        }
        return value;
    } catch (const std::exception&) {
        throw std::invalid_argument("Please enter a number");
    }
}

std::vector<std::string> parseTags(const std::string& line) {
    std::vector<std::string> tags;
    std::stringstream ss(line);
    std::string tag;
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
    std::cout << "3. Remove task\n";
    std::cout << "4. Mark task as done\n";
    std::cout << "5. Search by text\n";
    std::cout << "6. Search by tag\n";
    std::cout << "7. Sort by deadline\n";
    std::cout << "8. Show overdue tasks\n";
    std::cout << "9. Save to JSON\n";
    std::cout << "10. Load from JSON\n";
    std::cout << "11. Export to CSV\n";
    std::cout << "12. Import from CSV\n";
    std::cout << "0. Exit\n";
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
            case 1: {
                std::string title = readLine("Title: ");
                std::string deadline = readLine("Deadline (YYYY-MM-DD, empty to skip): ");
                std::string tagsLine = readLine("Tags (separated by space): ");
                int id = manager.addTask(title, deadline, parseTags(tagsLine));
                std::cout << "Task added with id " << id << "\n";
                break;
            }
            case 2:
                printTasks(manager.getAll());
                break;
            case 3: {
                int id = readInt("Task id: ");
                manager.removeTask(id);
                std::cout << "Task removed\n";
                break;
            }
            case 4: {
                int id = readInt("Task id: ");
                manager.markDone(id);
                std::cout << "Task marked as done\n";
                break;
            }
            case 5: {
                std::string text = readLine("Text: ");
                printTasks(manager.searchByText(text));
                break;
            }
            case 6: {
                std::vector<std::string> tags = parseTags(readLine("Tag: "));
                if (tags.empty()) {
                    throw std::invalid_argument("Tag cannot be empty");
                }
                printTasks(manager.searchByTag(tags[0]));
                break;
            }
            case 7:
                printTasks(manager.getSortedByDeadline());
                break;
            case 8:
                std::cout << "Today: " << getToday() << "\n";
                printTasks(manager.getOverdue(getToday()));
                break;
            case 9: {
                std::string filename = readLine("File name (empty = tasks.json): ");
                if (filename.empty()) {
                    filename = DEFAULT_FILE;
                }
                manager.saveToJson(filename);
                std::cout << "Saved to " << filename << "\n";
                break;
            }
            case 10: {
                std::string filename = readLine("File name (empty = tasks.json): ");
                if (filename.empty()) {
                    filename = DEFAULT_FILE;
                }
                manager.loadFromJson(filename);
                std::cout << "Loaded " << manager.getAll().size() << " tasks\n";
                break;
            }
            case 11: {
                std::string filename = readLine("File name (empty = tasks.csv): ");
                if (filename.empty()) {
                    filename = "tasks.csv";
                }
                manager.saveToCsv(filename);
                std::cout << "Exported to " << filename << "\n";
                break;
            }
            case 12: {
                std::string filename = readLine("File name (empty = tasks.csv): ");
                if (filename.empty()) {
                    filename = "tasks.csv";
                }
                manager.loadFromCsv(filename);
                std::cout << "Imported " << manager.getAll().size() << " tasks\n";
                break;
            }
            case 0:
                running = false;
                break;
            default:
                std::cout << "Unknown command\n";
            }
        } catch (const std::runtime_error& e) {
            if (std::string(e.what()) == "Input closed") {
                running = false;
            } else {
                std::cout << "Error: " << e.what() << "\n";
            }
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
