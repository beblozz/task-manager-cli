#pragma once

#include <string>
#include <vector>

#include "Task.h"

class TaskManager {
public:
    TaskManager();

    int addTask(const std::string& title, const std::string& deadline, const std::vector<std::string>& tags);
    void removeTask(int id);
    void markDone(int id);

    std::vector<Task> getAll() const;
    std::vector<Task> searchByText(const std::string& text) const;
    std::vector<Task> searchByTag(const std::string& tag) const;
    std::vector<Task> getSortedByDeadline() const;
    std::vector<Task> getOverdue(const std::string& today) const;

    void saveToJson(const std::string& filename) const;
    void loadFromJson(const std::string& filename);
    void saveToCsv(const std::string& filename) const;
    void loadFromCsv(const std::string& filename);

    static bool isValidDate(const std::string& date);

private:
    std::vector<Task> tasks;
    int nextId;

    int findIndex(int id) const;
    void updateNextId();
};
