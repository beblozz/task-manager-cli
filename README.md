# Task Manager: C++17

[![CI](https://github.com/beblozz/task-manager-cli/actions/workflows/ci.yml/badge.svg)](https://github.com/beblozz/task-manager-cli/actions/workflows/ci.yml)

Менеджер задач с консольным и веб-интерфейсом. Вся логика (хранение задач, поиск, приоритеты, валидация, сохранение в JSON/CSV) написана на C++ и используется в двух программах:

- **`task_manager`**: консольное приложение с меню
- **`task_web`**: HTTP-сервер на C++ с REST API и веб-страницей (HTML/CSS/JS)

Код покрыт unit-тестами (GoogleTest), которые автоматически запускаются на Linux и Windows через GitHub Actions.

## Возможности

- Добавление, **редактирование**, удаление и отметка задач как выполненных
- **Приоритеты**: низкий, средний, высокий; сортировка по приоритету
- Дедлайны с проверкой корректности даты (включая високосные годы)
- Теги и фильтрация по ним
- Поиск по названию без учёта регистра (латиница и кириллица в UTF-8)
- Сортировка по дедлайну, список просроченных задач
- Сохранение и загрузка в **JSON** (собственный парсер, без сторонних библиотек)
- Экспорт и импорт в **CSV** с корректной обработкой кавычек, запятых и переводов строк Windows
- Обратная совместимость: файлы версии 1.0 (без приоритетов) открываются без ошибок
- Обработка ошибок: неверный ввод, несуществующие задачи, повреждённые файлы, повторяющиеся id
- Веб-интерфейс со светлой и тёмной темой и адаптивной вёрсткой

## Технологии

| Область        | Что использовано |
|----------------|------------------|
| Язык           | C++17 |
| Сборка         | CMake |
| STL            | `std::vector`, `std::string`, `std::optional`, `std::stable_sort`, `std::fstream`, `std::filesystem`, `std::mutex` |
| ООП            | классы, инкапсуляция, разделение ответственности, собственный класс исключения |
| Ошибки         | исключения `std::invalid_argument`, `std::out_of_range`, `std::runtime_error` |
| Тесты          | [GoogleTest](https://github.com/google/googletest), подключается через CMake `FetchContent` |
| CI             | GitHub Actions: сборка и тесты на Ubuntu и Windows |
| Сеть           | [cpp-httplib](https://github.com/yhirose/cpp-httplib) (header-only, MIT) |
| Веб            | HTML, CSS, JavaScript (`fetch`, без фреймворков) |

## Архитектура

```
Браузер (HTML / CSS / JS)
        │  HTTP + JSON
        ▼
  task_web (server.cpp) ──┐
                          ├──► TaskManager ──► Task
  task_manager (main.cpp) ┘         │
                                    ▼
  task_tests (GoogleTest)   tasks.json / tasks.csv
```

- `Task`: одна задача (id, название, дедлайн, теги, приоритет, статус)
- `TaskManager`: коллекция задач, поиск, сортировка, редактирование, валидация, работа с файлами
- `Utils`: общие функции для консоли и сервера (сегодняшняя дата, разбор тегов)
- `main.cpp`: консольный интерфейс
- `server.cpp`: HTTP API; доступ к данным защищён `std::mutex`, так как сервер обрабатывает запросы в нескольких потоках

Классы `Task`, `TaskManager` и `Utils` собираются в библиотеку `task_core`, которую используют обе программы и тесты.

Редактирование работает по принципу «всё или ничего»: `TaskManager::editTask` сначала проверяет все новые значения и только потом меняет задачу. Если хоть одно поле неверное, задача остаётся как была.

## Сборка

Нужны компилятор с поддержкой C++17 (GCC, Clang, MSVC), CMake 3.14+ и интернет при первой сборке (CMake скачает GoogleTest).

```bash
git clone https://github.com/beblozz/task-manager-cli.git
cd task-manager-cli
cmake -S . -B build
cmake --build build
```

Собрать без тестов (не нужен интернет):

```bash
cmake -S . -B build -DTASK_BUILD_TESTS=OFF
```

## Тесты

```bash
cd build
ctest --output-on-failure
```

На Windows с Visual Studio: `ctest -C Debug --output-on-failure`.

55 тестов в четырёх файлах:

| Файл | Что проверяет |
|------|---------------|
| `tests/test_task.cpp` | значения по умолчанию, теги, просрочка, вывод в строку |
| `tests/test_task_manager.cpp` | добавление, удаление, редактирование, поиск, сортировки, проверка дат и приоритетов |
| `tests/test_storage.cpp` | сохранение и загрузка JSON/CSV, файлы версии 1.0, испорченные файлы |
| `tests/test_utils.cpp` | разбор тегов, формат даты |

## Запуск

**Консольная версия:**

```bash
./build/task_manager
```

**Веб-версия** (запускать из корня проекта):

```bash
./build/task_web
```

После запуска открыть http://localhost:8080

На Windows исполняемые файлы находятся в `build\` или `build\Debug\`, в зависимости от компилятора.

Задачи хранятся в `tasks.json` в текущей папке: загружаются при старте и сохраняются после каждого изменения.

## Пример работы в консоли

```
===== Task Manager =====
1. Add task
2. Show all tasks
3. Edit task
...
> 1
Title: Сдать лабу по C++
Deadline (YYYY-MM-DD, empty to skip): 2026-10-20
Tags (separated by space): study cpp
Priority (1 low, 2 medium, 3 high, empty = 2): 3
Task added with id 1

> 2
[1] [ ] [!!!] Сдать лабу по C++ | deadline: 2026-10-20 | tags: #study #cpp

> 3
Task id: 1
Now: [1] [ ] [!!!] Сдать лабу по C++ | deadline: 2026-10-20 | tags: #study #cpp
Press Enter to keep the current value
New title:
New deadline (YYYY-MM-DD, '-' to remove): 2026-10-25
New priority (1 low, 2 medium, 3 high):
Task updated: [1] [ ] [!!!] Сдать лабу по C++ | deadline: 2026-10-25 | tags: #study #cpp
```

## REST API

| Метод  | Путь                    | Описание |
|--------|-------------------------|----------|
| GET    | `/api/tasks`            | Список задач. Параметры: `q` (поиск), `tag`, `sort=deadline` или `sort=priority`, `overdue=1` |
| POST   | `/api/tasks`            | Создать задачу: `title`, `deadline`, `tags`, `priority` (1–3, по умолчанию 2) |
| PATCH  | `/api/tasks/{id}`       | Изменить задачу. Меняются только переданные поля: `title`, `deadline` (пустой = убрать), `priority` |
| POST   | `/api/tasks/{id}/done`  | Отметить задачу выполненной |
| DELETE | `/api/tasks/{id}`       | Удалить задачу |
| GET    | `/api/export/csv`       | Скачать задачи в CSV |

Ошибки возвращаются в формате `{"error": "..."}` с кодами `400` (неверные данные), `404` (задача не найдена), `500` (ошибка сервера).

Пример:

```bash
curl -X POST http://localhost:8080/api/tasks -d "title=Read book" -d "deadline=2026-10-10" -d "tags=study" -d "priority=3"
curl -X PATCH http://localhost:8080/api/tasks/1 -d "priority=1"
curl "http://localhost:8080/api/tasks?tag=study&sort=priority"
```

## Планы

- [x] Редактирование задач
- [x] Приоритеты задач
- [x] Unit-тесты на GoogleTest
- [x] CI через GitHub Actions
- [ ] Интерфейс хранилища `Storage` с реализациями для JSON и CSV
- [ ] Команды из командной строки: `task_manager add "..." --tag cpp`
- [ ] `std::chrono` для работы с датами

## Автор

**beblozz**, студент, изучаю C++.
GitHub: [@beblozz](https://github.com/beblozz)
