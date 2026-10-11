#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Task.h"

// Хранит все задачи и умеет с ними работать: добавлять, искать, сортировать,
// сохранять в JSON/CSV. При ошибке бросает исключение:
//   std::invalid_argument — неверные данные (пустое название, плохая дата, приоритет)
//   std::out_of_range     — задачи с таким id нет
//   std::runtime_error    — ошибка файла или формата
class TaskManager {
public:
    TaskManager();

    // Возвращает id новой задачи
    int addTask(const std::string& title, const std::string& deadline,
                const std::vector<std::string>& tags, int priority = Task::PRIORITY_MEDIUM);
    void removeTask(int id);
    void markDone(int id);

    // Меняет только те поля, которые переданы (std::nullopt = не трогать).
    // Пустой deadline убирает дедлайн. Сначала проверяет всё, потом меняет:
    // если хоть одно поле неверное, задача останется как была.
    void editTask(int id,
                  const std::optional<std::string>& title,
                  const std::optional<std::string>& deadline,
                  const std::optional<int>& priority);

    Task getTask(int id) const;
    std::vector<Task> getAll() const;
    std::vector<Task> searchByText(const std::string& text) const;
    std::vector<Task> searchByTag(const std::string& tag) const;
    std::vector<Task> getSortedByDeadline() const;
    // Сначала высокий приоритет, при равном приоритете — по дедлайну
    std::vector<Task> getSortedByPriority() const;
    std::vector<Task> getOverdue(const std::string& today) const;

    void saveToJson(const std::string& filename) const;
    void loadFromJson(const std::string& filename);
    void saveToCsv(const std::string& filename) const;
    void loadFromCsv(const std::string& filename);

    static bool isValidDate(const std::string& date);
    static bool isValidPriority(int priority);
    static std::string toJson(const std::vector<Task>& list);
    // Нижний регистр для латиницы и кириллицы в UTF-8
    static std::string toLower(const std::string& s);

private:
    std::vector<Task> tasks;
    int nextId;

    int findIndex(int id) const;
    void updateNextId();
};
