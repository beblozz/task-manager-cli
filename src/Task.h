#pragma once

#include <string>
#include <vector>

// Одна задача: номер, название, дедлайн, теги, приоритет и статус выполнения.
// Класс только хранит данные. Проверки (корректная дата, приоритет 1..3)
// делает TaskManager.
class Task {
public:
    // Значения приоритета
    static constexpr int PRIORITY_LOW = 1;
    static constexpr int PRIORITY_MEDIUM = 2;
    static constexpr int PRIORITY_HIGH = 3;

    Task();
    // priority можно не передавать, тогда задача будет со средним приоритетом
    Task(int id, const std::string& title, const std::string& deadline,
         const std::vector<std::string>& tags, int priority = PRIORITY_MEDIUM);

    int getId() const;
    std::string getTitle() const;
    std::string getDeadline() const;
    std::vector<std::string> getTags() const;
    int getPriority() const;
    bool isDone() const;

    void setId(int id);
    void setTitle(const std::string& title);
    void setDeadline(const std::string& deadline);
    void setTags(const std::vector<std::string>& tags);
    void setPriority(int priority);
    void setDone(bool done);

    bool hasTag(const std::string& tag) const;
    // Просрочена, если не выполнена и дедлайн раньше сегодняшней даты
    bool isOverdue(const std::string& today) const;
    // Строка для вывода в консоль
    std::string toString() const;

private:
    int id;
    std::string title;
    std::string deadline;  // "YYYY-MM-DD" или пустая строка, если дедлайна нет
    std::vector<std::string> tags;
    int priority;          // 1 — низкий, 2 — средний, 3 — высокий
    bool done;
};
