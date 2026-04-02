'use strict';

type rawint32 = number;

type Task = {
  id: rawint32;
  priority: rawint32;
  work: rawint32;
  next: Task | null;
};

let taskList: Task | null = null;

function addTask(id: rawint32, priority: rawint32, work: rawint32): void {
  const task: Task = { id, priority, work, next: null };
  if (taskList === null || priority > taskList.priority) {
    task.next = taskList;
    taskList = task;
    return;
  }
  let current = taskList;
  while (current.next && current.next.priority >= priority) {
    current = current.next;
  }
  task.next = current.next;
  current.next = task;
}

function setup(): void {
  taskList = null;
  for (let i: rawint32 = 0; i < 20; i = i + 1) {
    addTask(i, (i % 5) + 1, (i % 7) + 3);
  }
}

function runScheduler(iterations: rawint32): rawint32 {
  let checksum: rawint32 = 0;
  for (let i: rawint32 = 0; i < iterations; i = i + 1) {
    if (taskList === null) break;
    const task : Task = taskList;
    taskList = task.next;
    task.work = task.work - 1;
    checksum += task.id + task.work;
    if (task.work > 0) {
      addTask(task.id, task.priority, task.work);
    }
  }
  return checksum;
}

function bench(): rawint32 {
  return runScheduler(200);
}

function teardown(): void {
  taskList = null;
}
