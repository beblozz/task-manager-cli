# Task Manager CLI

Менеджер задач на C++17: консольная версия и веб-интерфейс.

Логика (задачи, поиск, JSON/CSV) написана на C++ один раз и используется в двух программах:

- `task_manager` — консольное меню
- `task_web` — маленький HTTP-сервер на C++, который отдаёт страницу (HTML/CSS/JS) и JSON API

## Возможности

- добавление и удаление задач
- дедлайны в формате `YYYY-MM-DD` с проверкой даты
- теги (`#study #cpp`)
- поиск по тексту и по тегу
- сортировка по дедлайну и список просроченных задач
- сохранение и загрузка в JSON
- экспорт и импорт в CSV
- обработка ошибок ввода и ошибок файлов
- поиск без учёта регистра, в том числе для русских букв
- веб-интерфейс со светлой и тёмной темой

## Сборка

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Или без CMake:

```bash
g++ -std=c++17 -Wall -Wextra src/main.cpp src/Task.cpp src/TaskManager.cpp -o task_manager
g++ -std=c++17 src/server.cpp src/Task.cpp src/TaskManager.cpp -o task_web -pthread
```

На Windows (MinGW) для `task_web` добавь в конец `-lws2_32`.

## Запуск

Консоль:

```bash
./build/task_manager
```

Веб-версия (запускать из корня проекта, чтобы сервер нашёл папку `web`):

```bash
./build/task_web
```

Потом открыть http://localhost:8080

Обе программы хранят задачи в `tasks.json` в текущей папке.

## API

| Метод  | Путь                    | Что делает                                   |
|--------|-------------------------|----------------------------------------------|
| GET    | `/api/tasks`            | список задач, параметры `q`, `tag`, `sort=deadline`, `overdue=1` |
| POST   | `/api/tasks`            | добавить задачу (`title`, `deadline`, `tags`) |
| POST   | `/api/tasks/{id}/done`  | отметить выполненной                         |
| DELETE | `/api/tasks/{id}`       | удалить                                      |
| GET    | `/api/export/csv`       | скачать CSV                                  |

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
  main.cpp                        консольное меню
  server.cpp                      HTTP-сервер и API
web/
  index.html                      разметка страницы
  css/style.css                   стили
  js/app.js                       запросы к API и отрисовка списка
third_party/
  httplib.h                       cpp-httplib (MIT), HTTP-сервер в одном заголовке
```

## Что планирую добавить

- редактирование задачи
- приоритеты
- unit-тесты
