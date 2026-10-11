#pragma once

#include <string>
#include <vector>

// Общие функции для консольной и веб-версии.
// Раньше они были скопированы в main.cpp и server.cpp — теперь лежат в одном месте.

// Сегодняшняя дата в формате YYYY-MM-DD
std::string getToday();

// "study #cpp  home" -> {"study", "cpp", "home"}
std::vector<std::string> parseTags(const std::string& line);
