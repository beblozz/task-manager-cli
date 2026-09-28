# Task Manager CLI

Консольный менеджер задач на C++17.

## Возможности

- добавление и удаление задач
- дедлайны в формате `YYYY-MM-DD` с проверкой даты
- теги (`#study #cpp`)
- поиск по тексту и по тегу
- сортировка по дедлайну и список просроченных задач
- сохранение и загрузка в JSON
- экспорт и импорт в CSV
- обработка ошибок ввода и ошибок файлов

## Сборка

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Или без CMake:

```bash
g++ -std=c++17 -Wall -Wextra src/*.cpp -o task_manager
```

## Запуск

```bash
./task_manager
```

При запуске программа загружает `tasks.json` (если он есть), при выходе сохраняет задачи обратно.

## Пример

```
===== Task Manager =====
1. Add task
...
> 1
Title: Сдать лабу по C++
Deadline (YYYY-MM-DD, empty to skip): 2026-10-05
Tags (separated by space): study cpp
Task added with id 1
> 2
[1] [ ] Сдать лабу по C++ | deadline: 2026-10-05 | tags: #study #cpp
```

## Структура

```
src/
  Task.h / Task.cpp               класс задачи
  TaskManager.h / TaskManager.cpp список задач, поиск, JSON и CSV
  main.cpp                        меню и ввод пользователя
```

## Что планирую добавить

- редактирование задачи
- приоритеты
- unit-тесты
