#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "TaskManager.h"

// Вспомогательная функция: id задач по порядку, чтобы удобно сравнивать списки
static std::vector<int> ids(const std::vector<Task>& tasks) {
    std::vector<int> result;
    for (const Task& task : tasks) {
        result.push_back(task.getId());
    }
    return result;
}

// ===== Добавление =====

TEST(TaskManagerTest, AddReturnsIncreasingIds) {
    TaskManager manager;
    EXPECT_EQ(manager.addTask("A", "", {}), 1);
    EXPECT_EQ(manager.addTask("B", "", {}), 2);
    EXPECT_EQ(manager.getAll().size(), 2u);
}

TEST(TaskManagerTest, AddWithPriority) {
    TaskManager manager;
    int id = manager.addTask("A", "", {}, Task::PRIORITY_HIGH);
    EXPECT_EQ(manager.getTask(id).getPriority(), Task::PRIORITY_HIGH);
}

TEST(TaskManagerTest, AddRejectsEmptyTitle) {
    TaskManager manager;
    EXPECT_THROW(manager.addTask("", "", {}), std::invalid_argument);
    EXPECT_TRUE(manager.getAll().empty());
}

TEST(TaskManagerTest, AddRejectsWrongDate) {
    TaskManager manager;
    EXPECT_THROW(manager.addTask("A", "2026-02-30", {}), std::invalid_argument);
    EXPECT_THROW(manager.addTask("A", "05.10.2026", {}), std::invalid_argument);
}

TEST(TaskManagerTest, AddRejectsWrongPriority) {
    TaskManager manager;
    EXPECT_THROW(manager.addTask("A", "", {}, 0), std::invalid_argument);
    EXPECT_THROW(manager.addTask("A", "", {}, 4), std::invalid_argument);
}

// ===== Удаление и выполнение =====

TEST(TaskManagerTest, RemoveTask) {
    TaskManager manager;
    manager.addTask("A", "", {});
    int id = manager.addTask("B", "", {});
    manager.removeTask(id);
    EXPECT_EQ(ids(manager.getAll()), std::vector<int>({1}));
}

TEST(TaskManagerTest, RemoveMissingTaskThrows) {
    TaskManager manager;
    EXPECT_THROW(manager.removeTask(42), std::out_of_range);
}

TEST(TaskManagerTest, IdsAreNotReusedAfterRemove) {
    TaskManager manager;
    manager.addTask("A", "", {});
    int id = manager.addTask("B", "", {});
    manager.removeTask(id);
    EXPECT_EQ(manager.addTask("C", "", {}), 3);
}

TEST(TaskManagerTest, MarkDone) {
    TaskManager manager;
    int id = manager.addTask("A", "", {});
    manager.markDone(id);
    EXPECT_TRUE(manager.getTask(id).isDone());
    EXPECT_THROW(manager.markDone(99), std::out_of_range);
}

// ===== Редактирование =====

TEST(TaskManagerTest, EditChangesOnlyGivenFields) {
    TaskManager manager;
    int id = manager.addTask("Старое", "2026-10-01", {"tag"}, Task::PRIORITY_LOW);

    manager.editTask(id, std::string("Новое"), std::nullopt, std::nullopt);

    Task task = manager.getTask(id);
    EXPECT_EQ(task.getTitle(), "Новое");
    EXPECT_EQ(task.getDeadline(), "2026-10-01");
    EXPECT_EQ(task.getPriority(), Task::PRIORITY_LOW);
}

TEST(TaskManagerTest, EditAllFields) {
    TaskManager manager;
    int id = manager.addTask("A", "", {});
    manager.editTask(id, std::string("B"), std::string("2026-12-31"), Task::PRIORITY_HIGH);

    Task task = manager.getTask(id);
    EXPECT_EQ(task.getTitle(), "B");
    EXPECT_EQ(task.getDeadline(), "2026-12-31");
    EXPECT_EQ(task.getPriority(), Task::PRIORITY_HIGH);
}

TEST(TaskManagerTest, EditEmptyDeadlineRemovesIt) {
    TaskManager manager;
    int id = manager.addTask("A", "2026-10-01", {});
    manager.editTask(id, std::nullopt, std::string(""), std::nullopt);
    EXPECT_TRUE(manager.getTask(id).getDeadline().empty());
}

// Если одно поле неверное, задача не должна измениться совсем
TEST(TaskManagerTest, EditIsAllOrNothing) {
    TaskManager manager;
    int id = manager.addTask("A", "", {});

    EXPECT_THROW(manager.editTask(id, std::string("B"), std::nullopt, 7), std::invalid_argument);
    EXPECT_EQ(manager.getTask(id).getTitle(), "A");

    EXPECT_THROW(manager.editTask(id, std::string("B"), std::string("2026-13-01"), std::nullopt),
                 std::invalid_argument);
    EXPECT_EQ(manager.getTask(id).getTitle(), "A");
}

TEST(TaskManagerTest, EditRejectsEmptyTitle) {
    TaskManager manager;
    int id = manager.addTask("A", "", {});
    EXPECT_THROW(manager.editTask(id, std::string(""), std::nullopt, std::nullopt), std::invalid_argument);
}

TEST(TaskManagerTest, EditMissingTaskThrows) {
    TaskManager manager;
    EXPECT_THROW(manager.editTask(5, std::string("A"), std::nullopt, std::nullopt), std::out_of_range);
}

// ===== Поиск =====

TEST(TaskManagerTest, SearchIgnoresCaseForLatin) {
    TaskManager manager;
    manager.addTask("Learn C++", "", {});
    manager.addTask("Buy milk", "", {});
    EXPECT_EQ(ids(manager.searchByText("LEARN")), std::vector<int>({1}));
}

TEST(TaskManagerTest, SearchIgnoresCaseForCyrillic) {
    TaskManager manager;
    manager.addTask("Сдать лабу", "", {});
    manager.addTask("Купить молоко", "", {});
    manager.addTask("Ёлка", "", {});
    EXPECT_EQ(ids(manager.searchByText("ЛАБ")), std::vector<int>({1}));
    EXPECT_EQ(ids(manager.searchByText("МОЛОКО")), std::vector<int>({2}));
    EXPECT_EQ(ids(manager.searchByText("ёлка")), std::vector<int>({3}));
}

TEST(TaskManagerTest, SearchByTag) {
    TaskManager manager;
    manager.addTask("A", "", {"study"});
    manager.addTask("B", "", {"home"});
    manager.addTask("C", "", {"study", "cpp"});
    EXPECT_EQ(ids(manager.searchByTag("study")), std::vector<int>({1, 3}));
    EXPECT_TRUE(manager.searchByTag("work").empty());
}

// ===== Сортировка и просроченные =====

TEST(TaskManagerTest, SortByDeadlinePutsEmptyLast) {
    TaskManager manager;
    manager.addTask("A", "", {});
    manager.addTask("B", "2026-12-01", {});
    manager.addTask("C", "2026-01-01", {});
    EXPECT_EQ(ids(manager.getSortedByDeadline()), std::vector<int>({3, 2, 1}));
}

TEST(TaskManagerTest, SortByPriorityThenDeadline) {
    TaskManager manager;
    manager.addTask("low", "2026-01-01", {}, Task::PRIORITY_LOW);
    manager.addTask("high late", "2026-12-01", {}, Task::PRIORITY_HIGH);
    manager.addTask("medium", "", {}, Task::PRIORITY_MEDIUM);
    manager.addTask("high early", "2026-02-01", {}, Task::PRIORITY_HIGH);
    EXPECT_EQ(ids(manager.getSortedByPriority()), std::vector<int>({4, 2, 3, 1}));
}

TEST(TaskManagerTest, SortDoesNotChangeOriginalOrder) {
    TaskManager manager;
    manager.addTask("A", "2026-12-01", {});
    manager.addTask("B", "2026-01-01", {});
    manager.getSortedByDeadline();
    EXPECT_EQ(ids(manager.getAll()), std::vector<int>({1, 2}));
}

TEST(TaskManagerTest, GetOverdue) {
    TaskManager manager;
    manager.addTask("past", "2026-01-01", {});
    manager.addTask("future", "2026-12-01", {});
    int doneId = manager.addTask("past but done", "2026-01-01", {});
    manager.markDone(doneId);
    EXPECT_EQ(ids(manager.getOverdue("2026-06-01")), std::vector<int>({1}));
}

// ===== Проверка даты =====

TEST(TaskManagerTest, ValidDates) {
    EXPECT_TRUE(TaskManager::isValidDate("2026-10-05"));
    EXPECT_TRUE(TaskManager::isValidDate("2026-12-31"));
    EXPECT_TRUE(TaskManager::isValidDate("2024-02-29"));  // високосный
    EXPECT_TRUE(TaskManager::isValidDate("2000-02-29"));  // делится на 400
}

TEST(TaskManagerTest, InvalidDates) {
    EXPECT_FALSE(TaskManager::isValidDate(""));
    EXPECT_FALSE(TaskManager::isValidDate("2026-1-5"));
    EXPECT_FALSE(TaskManager::isValidDate("05.10.2026"));
    EXPECT_FALSE(TaskManager::isValidDate("2026-00-10"));
    EXPECT_FALSE(TaskManager::isValidDate("2026-13-10"));
    EXPECT_FALSE(TaskManager::isValidDate("2026-04-31"));
    EXPECT_FALSE(TaskManager::isValidDate("2023-02-29"));  // не високосный
    EXPECT_FALSE(TaskManager::isValidDate("1900-02-29"));  // делится на 100, но не на 400
    EXPECT_FALSE(TaskManager::isValidDate("2026-ab-10"));
}

TEST(TaskManagerTest, ValidPriorities) {
    EXPECT_TRUE(TaskManager::isValidPriority(1));
    EXPECT_TRUE(TaskManager::isValidPriority(3));
    EXPECT_FALSE(TaskManager::isValidPriority(0));
    EXPECT_FALSE(TaskManager::isValidPriority(-1));
    EXPECT_FALSE(TaskManager::isValidPriority(4));
}

TEST(TaskManagerTest, ToLower) {
    EXPECT_EQ(TaskManager::toLower("Hello WORLD"), "hello world");
    EXPECT_EQ(TaskManager::toLower("ПРИВЕТ Мир"), "привет мир");
    EXPECT_EQ(TaskManager::toLower("АБВЯЁ"), "абвяё");
    EXPECT_EQ(TaskManager::toLower("123 !?"), "123 !?");
}
