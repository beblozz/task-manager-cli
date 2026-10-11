// Элементы страницы, с которыми работаем
const form = document.getElementById("task-form");
const titleInput = document.getElementById("title");
const deadlineInput = document.getElementById("deadline");
const priorityInput = document.getElementById("priority");
const tagsInput = document.getElementById("tags");
const errorText = document.getElementById("error");
const searchInput = document.getElementById("search");
const sortSelect = document.getElementById("sort");
const overdueCheckbox = document.getElementById("overdue");
const taskList = document.getElementById("task-list");
const emptyText = document.getElementById("empty");
const tagFilter = document.getElementById("tag-filter");
const tagName = document.getElementById("tag-name");
const clearTagButton = document.getElementById("clear-tag");

const PRIORITY_NAMES = { 1: "Низкий", 2: "Средний", 3: "Высокий" };

let currentTag = "";
// id задачи, которая сейчас редактируется (null — никакая)
let editingId = null;

function getToday() {
    const now = new Date();
    const month = String(now.getMonth() + 1).padStart(2, "0");
    const day = String(now.getDate()).padStart(2, "0");
    return now.getFullYear() + "-" + month + "-" + day;
}

// Запрос к серверу. Если сервер ответил ошибкой, бросаем исключение
// с текстом из {"error": "..."}
async function request(url, options) {
    const response = await fetch(url, options);
    const data = await response.json();
    if (!response.ok) {
        throw new Error(data.error || "Server error");
    }
    return data;
}

function showError(message) {
    errorText.textContent = message;
}

async function loadTasks() {
    const params = new URLSearchParams();
    if (searchInput.value.trim() !== "") {
        params.append("q", searchInput.value.trim());
    }
    if (currentTag !== "") {
        params.append("tag", currentTag);
    }
    if (sortSelect.value !== "") {
        params.append("sort", sortSelect.value);
    }
    if (overdueCheckbox.checked) {
        params.append("overdue", "1");
    }

    try {
        const tasks = await request("/api/tasks?" + params.toString());
        renderTasks(tasks);
    } catch (e) {
        showError("Не удалось загрузить задачи: " + e.message);
    }
}

// Создать элемент с классом и текстом. textContent безопаснее innerHTML:
// текст задачи никогда не выполнится как HTML-код
function createElement(tag, className, text) {
    const element = document.createElement(tag);
    if (className) {
        element.className = className;
    }
    if (text !== undefined) {
        element.textContent = text;
    }
    return element;
}

function createButton(text, className, title, onClick) {
    const button = createElement("button", "small " + className, text);
    button.type = "button";
    button.title = title;
    button.addEventListener("click", onClick);
    return button;
}

function renderTasks(tasks) {
    taskList.innerHTML = "";
    emptyText.classList.toggle("hidden", tasks.length > 0);

    for (const task of tasks) {
        if (task.id === editingId) {
            taskList.appendChild(renderEditForm(task));
        } else {
            taskList.appendChild(renderTask(task));
        }
    }
}

function renderTask(task) {
    const li = createElement("li", "task priority-" + task.priority);
    if (task.done) {
        li.classList.add("done");
    }
    if (!task.done && task.deadline !== "" && task.deadline < getToday()) {
        li.classList.add("overdue");
    }

    const body = createElement("div", "task-body");
    body.appendChild(createElement("div", "task-title", task.title));

    const meta = createElement("div", "task-meta");
    meta.appendChild(createElement("span", "badge priority-badge", PRIORITY_NAMES[task.priority]));
    if (task.deadline !== "") {
        meta.appendChild(createElement("span", "deadline", "до " + task.deadline));
    }
    for (const tag of task.tags) {
        const tagSpan = createElement("span", "tag", "#" + tag);
        tagSpan.addEventListener("click", () => setTag(tag));
        meta.appendChild(tagSpan);
    }
    body.appendChild(meta);
    li.appendChild(body);

    const actions = createElement("div", "actions");
    if (!task.done) {
        actions.appendChild(createButton("✓", "btn-done", "Выполнено", () => markDone(task.id)));
    }
    actions.appendChild(createButton("✎", "btn-edit", "Изменить", () => startEdit(task.id)));
    actions.appendChild(createButton("✕", "btn-delete", "Удалить", () => deleteTask(task.id)));
    li.appendChild(actions);

    return li;
}

// Вместо задачи показываем маленькую форму с текущими значениями
function renderEditForm(task) {
    const li = createElement("li", "task editing");
    const editForm = createElement("form", "edit-form");

    const title = createElement("input");
    title.type = "text";
    title.value = task.title;
    title.required = true;

    const row = createElement("div", "row");
    const deadline = createElement("input");
    deadline.type = "date";
    deadline.value = task.deadline;

    const priority = createElement("select");
    for (const value of [1, 2, 3]) {
        const option = createElement("option", "", PRIORITY_NAMES[value]);
        option.value = value;
        option.selected = value === task.priority;
        priority.appendChild(option);
    }
    row.appendChild(deadline);
    row.appendChild(priority);

    const buttons = createElement("div", "row");
    const saveButton = createElement("button", "", "Сохранить");
    saveButton.type = "submit";
    const cancelButton = createButton("Отмена", "btn-cancel", "Отмена", cancelEdit);
    buttons.appendChild(saveButton);
    buttons.appendChild(cancelButton);

    editForm.appendChild(title);
    editForm.appendChild(row);
    editForm.appendChild(buttons);
    editForm.addEventListener("submit", (event) => {
        event.preventDefault();
        saveEdit(task.id, title.value.trim(), deadline.value, priority.value);
    });

    li.appendChild(editForm);
    // Сразу ставим курсор в название
    setTimeout(() => title.focus(), 0);
    return li;
}

async function addTask(event) {
    event.preventDefault();
    showError("");

    const body = new URLSearchParams();
    body.append("title", titleInput.value.trim());
    body.append("deadline", deadlineInput.value);
    body.append("tags", tagsInput.value);
    body.append("priority", priorityInput.value);

    try {
        await request("/api/tasks", { method: "POST", body: body });
        form.reset();
        titleInput.focus();
        loadTasks();
    } catch (e) {
        showError(e.message);
    }
}

function startEdit(id) {
    showError("");
    editingId = id;
    loadTasks();
}

function cancelEdit() {
    editingId = null;
    showError("");
    loadTasks();
}

async function saveEdit(id, title, deadline, priority) {
    const body = new URLSearchParams();
    body.append("title", title);
    body.append("deadline", deadline);
    body.append("priority", priority);

    try {
        await request("/api/tasks/" + id, { method: "PATCH", body: body });
        editingId = null;
        showError("");
        loadTasks();
    } catch (e) {
        showError(e.message);
    }
}

async function markDone(id) {
    try {
        await request("/api/tasks/" + id + "/done", { method: "POST" });
        loadTasks();
    } catch (e) {
        showError(e.message);
    }
}

async function deleteTask(id) {
    try {
        await request("/api/tasks/" + id, { method: "DELETE" });
        loadTasks();
    } catch (e) {
        showError(e.message);
    }
}

function setTag(tag) {
    currentTag = tag;
    tagName.textContent = "#" + tag;
    tagFilter.classList.toggle("hidden", tag === "");
    loadTasks();
}

form.addEventListener("submit", addTask);
searchInput.addEventListener("input", loadTasks);
sortSelect.addEventListener("change", loadTasks);
overdueCheckbox.addEventListener("change", loadTasks);
clearTagButton.addEventListener("click", () => setTag(""));

loadTasks();
