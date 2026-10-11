#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../third_party/httplib.h"
#include "TaskManager.h"
#include "Utils.h"

const std::string DATA_FILE = "tasks.json";
const std::string CSV_FILE = "tasks.csv";
const int PORT = 8080;

TaskManager manager;
// Сервер обрабатывает запросы в нескольких потоках одновременно.
// Мьютекс не даёт двум запросам менять задачи в один момент.
std::mutex managerMutex;

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

// Приоритет из параметра запроса. Если это не число — 400 Bad Request
int parsePriority(const std::string& value) {
    try {
        size_t pos = 0;
        int priority = std::stoi(value, &pos);
        if (pos == value.size()) {
            return priority;
        }
    } catch (const std::exception&) {
    }
    throw std::invalid_argument("Priority must be a number");
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

    // Файлы из папки web отдаются как есть: index.html, css, js
    if (!server.set_mount_point("/", "./web")) {
        std::cout << "Folder 'web' not found. Run the program from the project folder\n";
        return 1;
    }

    // GET /api/tasks?q=...&tag=...&sort=deadline|priority&overdue=1
    server.Get("/api/tasks", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);

        std::string text = TaskManager::toLower(req.get_param_value("q"));
        std::string tag = req.get_param_value("tag");
        std::string sort = req.get_param_value("sort");
        bool overdueOnly = req.get_param_value("overdue") == "1";
        std::string today = getToday();

        std::vector<Task> all;
        if (sort == "deadline") {
            all = manager.getSortedByDeadline();
        } else if (sort == "priority") {
            all = manager.getSortedByPriority();
        } else {
            all = manager.getAll();
        }

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

    // POST /api/tasks — создать задачу (title, deadline, tags, priority)
    server.Post("/api/tasks", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);
        try {
            int priority = Task::PRIORITY_MEDIUM;
            if (req.has_param("priority") && !req.get_param_value("priority").empty()) {
                priority = parsePriority(req.get_param_value("priority"));
            }
            int id = manager.addTask(req.get_param_value("title"),
                                     req.get_param_value("deadline"),
                                     parseTags(req.get_param_value("tags")),
                                     priority);
            saveData();
            res.status = 201;
            res.set_content("{\"id\": " + std::to_string(id) + "}", "application/json");
        } catch (const std::invalid_argument& e) {
            sendError(res, 400, e.what());
        } catch (const std::exception& e) {
            sendError(res, 500, e.what());
        }
    });

    // PATCH /api/tasks/{id} — изменить задачу.
    // Меняются только переданные поля: title, deadline (пустой = убрать), priority
    server.Patch(R"(/api/tasks/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(managerMutex);
        try {
            int id = std::stoi(req.matches[1]);

            std::optional<std::string> title;
            if (req.has_param("title")) {
                title = req.get_param_value("title");
            }
            std::optional<std::string> deadline;
            if (req.has_param("deadline")) {
                deadline = req.get_param_value("deadline");
            }
            std::optional<int> priority;
            if (req.has_param("priority")) {
                priority = parsePriority(req.get_param_value("priority"));
            }

            manager.editTask(id, title, deadline, priority);
            saveData();
            res.set_content(TaskManager::toJson({manager.getTask(id)}), "application/json");
        } catch (const std::invalid_argument& e) {
            sendError(res, 400, e.what());
        } catch (const std::out_of_range& e) {
            sendError(res, 404, e.what());
        } catch (const std::exception& e) {
            sendError(res, 500, e.what());
        }
    });

    // POST /api/tasks/{id}/done — отметить выполненной
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

    // DELETE /api/tasks/{id} — удалить
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

    // GET /api/export/csv — скачать все задачи в CSV
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
    // 127.0.0.1 — сервер доступен только с этого компьютера
    if (!server.listen("127.0.0.1", PORT)) {
        std::cout << "Could not start server on port " << PORT << "\n";
        return 1;
    }
    return 0;
}
