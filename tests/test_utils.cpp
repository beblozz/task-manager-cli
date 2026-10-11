#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "TaskManager.h"
#include "Utils.h"

TEST(UtilsTest, ParseTagsSplitsBySpaces) {
    EXPECT_EQ(parseTags("study cpp"), std::vector<std::string>({"study", "cpp"}));
}

TEST(UtilsTest, ParseTagsRemovesHashAndExtraSpaces) {
    EXPECT_EQ(parseTags("  #study   #cpp home "), std::vector<std::string>({"study", "cpp", "home"}));
}

TEST(UtilsTest, ParseTagsEmpty) {
    EXPECT_TRUE(parseTags("").empty());
    EXPECT_TRUE(parseTags("   ").empty());
    // Одиночная решётка — не тег
    EXPECT_TRUE(parseTags("#").empty());
}

TEST(UtilsTest, TodayHasValidFormat) {
    EXPECT_TRUE(TaskManager::isValidDate(getToday()));
}
