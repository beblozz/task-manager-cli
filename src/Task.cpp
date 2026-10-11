#include "Task.h"

// int и bool без начального значения содержат мусор, поэтому задаём их явно
Task::Task() : id(0), priority(PRIORITY_MEDIUM), done(false) {}

Task::Task(int id, const std::string& title, const std::string& deadline,
           const std::vector<std::string>& tags, int priority)
    : id(id), title(title), deadline(deadline), tags(tags), priority(priority), done(false) {}

int Task::getId() const { return id; }
std::string Task::getTitle() const { return title; }
std::string Task::getDeadline() const { return deadline; }
std::vector<std::string> Task::getTags() const { return tags; }
int Task::getPriority() const { return priority; }
bool Task::isDone() const { return done; }

void Task::setId(int id) { this->id = id; }
void Task::setTitle(const std::string& title) { this->title = title; }
void Task::setDeadline(const std::string& deadline) { this->deadline = deadline; }
void Task::setTags(const std::vector<std::string>& tags) { this->tags = tags; }
void Task::setPriority(int priority) { this->priority = priority; }
void Task::setDone(bool done) { this->done = done; }

bool Task::hasTag(const std::string& tag) const {
    for (const std::string& t : tags) {
        if (t == tag) {
            return true;
        }
    }
    return false;
}

bool Task::isOverdue(const std::string& today) const {
    if (done || deadline.empty()) {
        return false;
    }
    // Даты в формате YYYY-MM-DD можно сравнивать как обычные строки
    return deadline < today;
}

std::string Task::toString() const {
    std::string result = "[" + std::to_string(id) + "] ";
    result += done ? "[x] " : "[ ] ";

    // Средний приоритет не помечаем, чтобы список не пестрил
    if (priority == PRIORITY_HIGH) {
        result += "[!!!] ";
    } else if (priority == PRIORITY_LOW) {
        result += "[!] ";
    }

    result += title;
    if (!deadline.empty()) {
        result += " | deadline: " + deadline;
    }
    if (!tags.empty()) {
        result += " | tags:";
        for (const std::string& t : tags) {
            result += " #" + t;
        }
    }
    return result;
}
