#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "../third_party/httplib.h"
#include "TaskManager.h"

const std::string DATA_FILE = "tasks.json";
const std::string CSV_FILE = "tasks.csv";
const int PORT = 8080;

TaskManager manager;
std::mutex managerMutex;

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

std::string errorJson(const std::string& message) {
    std::string escaped;
    for (char c : message) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        escaped += c;
    }
    return "{\"error\": \"" + escaped + "\"}";
}

void sendError(httplib::Response& res, int status, const std::string& message) {
    res.status = status;
    res.set_content(errorJson(message), "application/json");
}

void saveData() {
    manager.saveToJson(DATA_FILE);
}

int main() {
    std::ifstream check(DATA_FILE);
    if (check.good()) {
        check.close();
        try {
            manager.loadFromJson(DATA_FILE);
            std::cout << "Loaded " << manager.getAll().size() << " tasks from " << DATA_FILE << "\n";
        } catch (const std::exception& e) {
            std::cout << "Could not load " << DATA_FILE << ": " << e.what() << "\n";
            std::cout << "Fix or remove the file and start again\n";
            return 1;
        }
    }

    httplib::Server server;

    if (!server.set_mount_point("/", "./web")) {
        std::cout << "Folder 'web' not found. Run the program from the project folder\n";
        return 1;
    }

    server.Get("/api/tasks", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);

        std::string text = TaskManager::toLower(req.get_param_value("q"));
        std::string tag = req.get_param_value("tag");
        bool overdueOnly = req.get_param_value("overdue") == "1";
        bool sortByDeadline = req.get_param_value("sort") == "deadline";
        std::string today = getToday();

        std::vector<Task> all = sortByDeadline ? manager.getSortedByDeadline() : manager.getAll();
        std::vector<Task> result;
        for (const Task& task : all) {
            if (!text.empty() && TaskManager::toLower(task.getTitle()).find(text) == std::string::npos) {
                continue;
            }
            if (!tag.empty() && !task.hasTag(tag)) {
                continue;
            }
            if (overdueOnly && !task.isOverdue(today)) {
                continue;
            }
            result.push_back(task);
        }
        res.set_content(TaskManager::toJson(result), "application/json");
    });

    server.Post("/api/tasks", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);
        try {
            std::string title = req.get_param_value("title");
            std::string deadline = req.get_param_value("deadline");
            std::vector<std::string> tags = parseTags(req.get_param_value("tags"));
            int id = manager.addTask(title, deadline, tags);
            saveData();
            res.status = 201;
            res.set_content("{\"id\": " + std::to_string(id) + "}", "application/json");
        } catch (const std::invalid_argument& e) {
            sendError(res, 400, e.what());
        } catch (const std::exception& e) {
            sendError(res, 500, e.what());
        }
    });

    server.Post(R"(/api/tasks/(\d+)/done)", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);
        try {
            int id = std::stoi(req.matches[1]);
            manager.markDone(id);
            saveData();
            res.set_content("{\"ok\": true}", "application/json");
        } catch (const std::out_of_range& e) {
            sendError(res, 404, e.what());
        } catch (const std::exception& e) {
            sendError(res, 500, e.what());
        }
    });

    server.Delete(R"(/api/tasks/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);
        try {
            int id = std::stoi(req.matches[1]);
            manager.removeTask(id);
            saveData();
            res.set_content("{\"ok\": true}", "application/json");
        } catch (const std::out_of_range& e) {
            sendError(res, 404, e.what());
        } catch (const std::exception& e) {
            sendError(res, 500, e.what());
        }
    });

    server.Get("/api/export/csv", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);
        try {
            manager.saveToCsv(CSV_FILE);
            std::ifstream file(CSV_FILE);
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Disposition", "attachment; filename=\"tasks.csv\"");
            res.set_content(buffer.str(), "text/csv; charset=utf-8");
        } catch (const std::exception& e) {
            sendError(res, 500, e.what());
        }
    });

    std::cout << "Open http://localhost:" << PORT << " in your browser\n";
    std::cout << "Press Ctrl+C to stop\n";
    if (!server.listen("127.0.0.1", PORT)) {
        std::cout << "Could not start server on port " << PORT << "\n";
        return 1;
    }
    return 0;
}
