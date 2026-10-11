#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "TaskManager.h"

namespace fs = std::filesystem;

// Фикстура: перед каждым тестом создаёт пустую временную папку,
// после теста удаляет её. Так тесты не мусорят и не мешают друг другу.
class StorageTest : public ::testing::Test {
protected:
    fs::path dir;

    void SetUp() override {
        const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir = fs::temp_directory_path() / ("task_manager_test_" + std::string(info->name()));
        fs::remove_all(dir);
        fs::create_directories(dir);
    }

    void TearDown() override {
        fs::remove_all(dir);
    }

    std::string path(const std::string& name) const {
        return (dir / name).string();
    }

    void writeFile(const std::string& name, const std::string& content) const {
        std::ofstream file(path(name), std::ios::binary);
        file << content;
    }
};

// Сохранили и загрузили — все поля должны совпасть
static void expectSameTasks(const TaskManager& a, const TaskManager& b) {
    std::vector<Task> x = a.getAll();
    std::vector<Task> y = b.getAll();
    ASSERT_EQ(x.size(), y.size());
    for (size_t i = 0; i < x.size(); i++) {
        EXPECT_EQ(x[i].getId(), y[i].getId());
        EXPECT_EQ(x[i].getTitle(), y[i].getTitle());
        EXPECT_EQ(x[i].getDeadline(), y[i].getDeadline());
        EXPECT_EQ(x[i].getTags(), y[i].getTags());
        EXPECT_EQ(x[i].getPriority(), y[i].getPriority());
        EXPECT_EQ(x[i].isDone(), y[i].isDone());
    }
}

static TaskManager makeSampleManager() {
    TaskManager manager;
    manager.addTask("Сдать лабу \"C++\"", "2026-10-05", {"study", "cpp"}, Task::PRIORITY_HIGH);
    manager.addTask("Купить хлеб, молоко", "", {"home"}, Task::PRIORITY_LOW);
    manager.addTask("Путь C:\\temp\\file", "2026-12-31", {});
    manager.markDone(2);
    return manager;
}

// ===== JSON =====

TEST_F(StorageTest, JsonRoundTrip) {
    TaskManager original = makeSampleManager();
    original.saveToJson(path("tasks.json"));

    TaskManager loaded;
    loaded.loadFromJson(path("tasks.json"));
    expectSameTasks(original, loaded);
}

TEST_F(StorageTest, JsonNextIdContinuesAfterLoad) {
    makeSampleManager().saveToJson(path("tasks.json"));
    TaskManager loaded;
    loaded.loadFromJson(path("tasks.json"));
    EXPECT_EQ(loaded.addTask("Новая", "", {}), 4);
}

// Файлы версии 1.0 без поля priority должны открываться
TEST_F(StorageTest, JsonFromVersion10WithoutPriority) {
    writeFile("old.json", R"([
  {
    "id": 3,
    "title": "Старая задача",
    "deadline": "2026-10-05",
    "done": false,
    "tags": ["study"]
  }
])");
    TaskManager manager;
    manager.loadFromJson(path("old.json"));
    ASSERT_EQ(manager.getAll().size(), 1u);
    EXPECT_EQ(manager.getTask(3).getPriority(), Task::PRIORITY_MEDIUM);
    EXPECT_EQ(manager.getTask(3).getTitle(), "Старая задача");
}

TEST_F(StorageTest, JsonEmptyArray) {
    writeFile("empty.json", "[]");
    TaskManager manager;
    manager.loadFromJson(path("empty.json"));
    EXPECT_TRUE(manager.getAll().empty());
}

// Испорченный файл: ошибка, а старые задачи остаются на месте
TEST_F(StorageTest, JsonBrokenFileKeepsCurrentTasks) {
    writeFile("broken.json", "[{\"id\": 1, \"title\": ");
    TaskManager manager;
    manager.addTask("Моя задача", "", {});

    EXPECT_THROW(manager.loadFromJson(path("broken.json")), std::runtime_error);
    ASSERT_EQ(manager.getAll().size(), 1u);
    EXPECT_EQ(manager.getAll()[0].getTitle(), "Моя задача");
}

TEST_F(StorageTest, JsonRejectsWrongPriority) {
    writeFile("bad.json", R"([{"id": 1, "title": "A", "deadline": "", "done": false, "priority": 9, "tags": []}])");
    TaskManager manager;
    EXPECT_THROW(manager.loadFromJson(path("bad.json")), std::runtime_error);
}

TEST_F(StorageTest, JsonRejectsWrongDate) {
    writeFile("bad.json", R"([{"id": 1, "title": "A", "deadline": "2026-02-30", "done": false, "tags": []}])");
    TaskManager manager;
    EXPECT_THROW(manager.loadFromJson(path("bad.json")), std::runtime_error);
}

TEST_F(StorageTest, JsonRejectsDuplicateIds) {
    writeFile("dup.json", R"([{"id": 1, "title": "A"}, {"id": 1, "title": "B"}])");
    TaskManager manager;
    EXPECT_THROW(manager.loadFromJson(path("dup.json")), std::runtime_error);
}

TEST_F(StorageTest, JsonMissingFileThrows) {
    TaskManager manager;
    EXPECT_THROW(manager.loadFromJson(path("nope.json")), std::runtime_error);
}

// ===== CSV =====

TEST_F(StorageTest, CsvRoundTrip) {
    TaskManager original = makeSampleManager();
    original.saveToCsv(path("tasks.csv"));

    TaskManager loaded;
    loaded.loadFromCsv(path("tasks.csv"));
    expectSameTasks(original, loaded);
}

// Файлы версии 1.0: 5 столбцов, без priority
TEST_F(StorageTest, CsvFromVersion10WithoutPriority) {
    writeFile("old.csv",
              "id,title,deadline,done,tags\n"
              "1,\"Сдать лабу, срочно\",2026-10-05,0,study;cpp\n"
              "2,Купить молоко,,1,home\n");
    TaskManager manager;
    manager.loadFromCsv(path("old.csv"));
    ASSERT_EQ(manager.getAll().size(), 2u);
    EXPECT_EQ(manager.getTask(1).getTitle(), "Сдать лабу, срочно");
    EXPECT_EQ(manager.getTask(1).getTags(), std::vector<std::string>({"study", "cpp"}));
    EXPECT_EQ(manager.getTask(1).getPriority(), Task::PRIORITY_MEDIUM);
    EXPECT_TRUE(manager.getTask(2).isDone());
}

// Файл, сохранённый в Windows: строки кончаются на \r\n
TEST_F(StorageTest, CsvWithWindowsLineEndings) {
    writeFile("win.csv",
              "id,title,deadline,done,tags\r\n"
              "1,A,2026-10-05,1,study\r\n");
    TaskManager manager;
    manager.loadFromCsv(path("win.csv"));
    // Без удаления \r тег стал бы "study\r"
    EXPECT_EQ(manager.getTask(1).getTags(), std::vector<std::string>({"study"}));
    EXPECT_TRUE(manager.getTask(1).isDone());
}

TEST_F(StorageTest, CsvRejectsWrongFieldCount) {
    writeFile("bad.csv", "id,title,deadline,done,tags,priority\n1,A,2026-10-05\n");
    TaskManager manager;
    EXPECT_THROW(manager.loadFromCsv(path("bad.csv")), std::runtime_error);
}

TEST_F(StorageTest, CsvRejectsUnclosedQuote) {
    writeFile("bad.csv", "id,title,deadline,done,tags\n1,\"A,2026-10-05,0,\n");
    TaskManager manager;
    EXPECT_THROW(manager.loadFromCsv(path("bad.csv")), std::runtime_error);
}

TEST_F(StorageTest, CsvRejectsWrongId) {
    writeFile("bad.csv", "id,title,deadline,done,tags\nabc,A,,0,\n");
    TaskManager manager;
    EXPECT_THROW(manager.loadFromCsv(path("bad.csv")), std::runtime_error);
}
