#include <gtest/gtest.h>

#include "Task.h"

// Новая задача по умолчанию: не выполнена, средний приоритет
TEST(TaskTest, DefaultValues) {
    Task task;
    EXPECT_EQ(task.getId(), 0);
    EXPECT_FALSE(task.isDone());
    EXPECT_EQ(task.getPriority(), Task::PRIORITY_MEDIUM);
    EXPECT_TRUE(task.getTitle().empty());
    EXPECT_TRUE(task.getTags().empty());
}

TEST(TaskTest, ConstructorStoresAllFields) {
    Task task(5, "Сдать лабу", "2026-10-05", {"study", "cpp"}, Task::PRIORITY_HIGH);
    EXPECT_EQ(task.getId(), 5);
    EXPECT_EQ(task.getTitle(), "Сдать лабу");
    EXPECT_EQ(task.getDeadline(), "2026-10-05");
    EXPECT_EQ(task.getTags().size(), 2u);
    EXPECT_EQ(task.getPriority(), Task::PRIORITY_HIGH);
    EXPECT_FALSE(task.isDone());
}

TEST(TaskTest, ConstructorWithoutPriorityUsesMedium) {
    Task task(1, "Задача", "", {});
    EXPECT_EQ(task.getPriority(), Task::PRIORITY_MEDIUM);
}

TEST(TaskTest, HasTag) {
    Task task(1, "Задача", "", {"study", "cpp"});
    EXPECT_TRUE(task.hasTag("cpp"));
    EXPECT_FALSE(task.hasTag("home"));
    // Тег должен совпасть целиком, а не частично
    EXPECT_FALSE(task.hasTag("cp"));
}

TEST(TaskTest, OverdueWhenDeadlineInPast) {
    Task task(1, "Задача", "2026-01-01", {});
    EXPECT_TRUE(task.isOverdue("2026-01-02"));
}

TEST(TaskTest, NotOverdueOnDeadlineDay) {
    Task task(1, "Задача", "2026-01-01", {});
    EXPECT_FALSE(task.isOverdue("2026-01-01"));
}

TEST(TaskTest, NotOverdueWhenDone) {
    Task task(1, "Задача", "2020-01-01", {});
    task.setDone(true);
    EXPECT_FALSE(task.isOverdue("2026-01-01"));
}

TEST(TaskTest, NotOverdueWithoutDeadline) {
    Task task(1, "Задача", "", {});
    EXPECT_FALSE(task.isOverdue("2026-01-01"));
}

TEST(TaskTest, ToStringShowsPriorityMarks) {
    Task high(1, "A", "", {}, Task::PRIORITY_HIGH);
    Task medium(2, "B", "", {}, Task::PRIORITY_MEDIUM);
    Task low(3, "C", "", {}, Task::PRIORITY_LOW);

    EXPECT_NE(high.toString().find("[!!!]"), std::string::npos);
    EXPECT_EQ(medium.toString().find("[!"), std::string::npos);
    EXPECT_NE(low.toString().find("[!] "), std::string::npos);
}

TEST(TaskTest, ToStringFullFormat) {
    Task task(7, "Сдать лабу", "2026-10-05", {"study", "cpp"});
    task.setDone(true);
    EXPECT_EQ(task.toString(), "[7] [x] Сдать лабу | deadline: 2026-10-05 | tags: #study #cpp");
}
