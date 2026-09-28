#include "Task.h"

Task::Task() : id(0), done(false) {}

Task::Task(int id, const std::string& title, const std::string& deadline, const std::vector<std::string>& tags)
    : id(id), title(title), deadline(deadline), tags(tags), done(false) {}

int Task::getId() const { return id; }
std::string Task::getTitle() const { return title; }
std::string Task::getDeadline() const { return deadline; }
std::vector<std::string> Task::getTags() const { return tags; }
bool Task::isDone() const { return done; }

void Task::setId(int id) { this->id = id; }
void Task::setTitle(const std::string& title) { this->title = title; }
void Task::setDeadline(const std::string& deadline) { this->deadline = deadline; }
void Task::setTags(const std::vector<std::string>& tags) { this->tags = tags; }
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
    return deadline < today;
}

std::string Task::toString() const {
    std::string result = "[" + std::to_string(id) + "] ";
    result += done ? "[x] " : "[ ] ";
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
