const form = document.getElementById("task-form");
const titleInput = document.getElementById("title");
const deadlineInput = document.getElementById("deadline");
const tagsInput = document.getElementById("tags");
const errorText = document.getElementById("error");
const searchInput = document.getElementById("search");
const sortCheckbox = document.getElementById("sort");
const overdueCheckbox = document.getElementById("overdue");
const taskList = document.getElementById("task-list");
const emptyText = document.getElementById("empty");
const tagFilter = document.getElementById("tag-filter");
const tagName = document.getElementById("tag-name");
const clearTagButton = document.getElementById("clear-tag");

let currentTag = "";

function getToday() {
    const now = new Date();
    const month = String(now.getMonth() + 1).padStart(2, "0");
    const day = String(now.getDate()).padStart(2, "0");
    return now.getFullYear() + "-" + month + "-" + day;
}

async function request(url, options) {
    const response = await fetch(url, options);
    const data = await response.json();
    if (!response.ok) {
        throw new Error(data.error || "Server error");
    }
    return data;
}

async function loadTasks() {
    const params = new URLSearchParams();
    if (searchInput.value.trim() !== "") {
        params.append("q", searchInput.value.trim());
    }
    if (currentTag !== "") {
        params.append("tag", currentTag);
    }
    if (sortCheckbox.checked) {
        params.append("sort", "deadline");
    }
    if (overdueCheckbox.checked) {
        params.append("overdue", "1");
    }

    try {
        const tasks = await request("/api/tasks?" + params.toString());
        renderTasks(tasks);
    } catch (e) {
        errorText.textContent = "Не удалось загрузить задачи: " + e.message;
    }
}

function renderTasks(tasks) {
    taskList.innerHTML = "";
    emptyText.classList.toggle("hidden", tasks.length > 0);

    const today = getToday();

    for (const task of tasks) {
        const li = document.createElement("li");
        li.className = "task";
        if (task.done) {
            li.classList.add("done");
        }
        if (!task.done && task.deadline !== "" && task.deadline < today) {
            li.classList.add("overdue");
        }

        const body = document.createElement("div");
        body.className = "task-body";

        const title = document.createElement("div");
        title.className = "task-title";
        title.textContent = task.title;
        body.appendChild(title);

        const meta = document.createElement("div");
        meta.className = "task-meta";

        if (task.deadline !== "") {
            const deadline = document.createElement("span");
            deadline.className = "deadline";
            deadline.textContent = "до " + task.deadline;
            meta.appendChild(deadline);
        }

        for (const tag of task.tags) {
            const tagSpan = document.createElement("span");
            tagSpan.className = "tag";
            tagSpan.textContent = "#" + tag;
            tagSpan.addEventListener("click", () => setTag(tag));
            meta.appendChild(tagSpan);
        }

        body.appendChild(meta);
        li.appendChild(body);

        const actions = document.createElement("div");
        actions.className = "actions";

        if (!task.done) {
            const doneButton = document.createElement("button");
            doneButton.className = "small btn-done";
            doneButton.textContent = "✓";
            doneButton.title = "Выполнено";
            doneButton.addEventListener("click", () => markDone(task.id));
            actions.appendChild(doneButton);
        }

        const deleteButton = document.createElement("button");
        deleteButton.className = "small btn-delete";
        deleteButton.textContent = "✕";
        deleteButton.title = "Удалить";
        deleteButton.addEventListener("click", () => deleteTask(task.id));
        actions.appendChild(deleteButton);

        li.appendChild(actions);
        taskList.appendChild(li);
    }
}

async function addTask(event) {
    event.preventDefault();
    errorText.textContent = "";

    const body = new URLSearchParams();
    body.append("title", titleInput.value.trim());
    body.append("deadline", deadlineInput.value);
    body.append("tags", tagsInput.value);

    try {
        await request("/api/tasks", { method: "POST", body: body });
        form.reset();
        titleInput.focus();
        loadTasks();
    } catch (e) {
        errorText.textContent = e.message;
    }
}

async function markDone(id) {
    try {
        await request("/api/tasks/" + id + "/done", { method: "POST" });
        loadTasks();
    } catch (e) {
        errorText.textContent = e.message;
    }
}

async function deleteTask(id) {
    try {
        await request("/api/tasks/" + id, { method: "DELETE" });
        loadTasks();
    } catch (e) {
        errorText.textContent = e.message;
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
sortCheckbox.addEventListener("change", loadTasks);
overdueCheckbox.addEventListener("change", loadTasks);
clearTagButton.addEventListener("click", () => setTag(""));

loadTasks();
