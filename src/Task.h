#pragma once

#include <string>
#include <vector>

class Task {
public:
    Task();
    Task(int id, const std::string& title, const std::string& deadline, const std::vector<std::string>& tags);

    int getId() const;
    std::string getTitle() const;
    std::string getDeadline() const;
    std::vector<std::string> getTags() const;
    bool isDone() const;

    void setId(int id);
    void setTitle(const std::string& title);
    void setDeadline(const std::string& deadline);
    void setTags(const std::vector<std::string>& tags);
    void setDone(bool done);

    bool hasTag(const std::string& tag) const;
    bool isOverdue(const std::string& today) const;
    std::string toString() const;

private:
    int id;
    std::string title;
    std::string deadline;
    std::vector<std::string> tags;
    bool done;
};
