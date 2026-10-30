// SPDX-License-Identifier: GPL-3.0-or-later
// Optional Node-only DOM simulation. Does NOT establish real-browser/OBS support.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import vm from 'node:vm';

const root = new URL('../', import.meta.url);
const html = readFileSync(new URL('PROGRESS.html', root), 'utf8');
const elements = new Map();
const element = id => {
  if (!elements.has(id)) elements.set(id, {
    value: '', hidden: false, textContent: '', listeners: {}, children: [],
    addEventListener(event, callback) { this.listeners[event] = callback; },
    appendChild(child) { this.children = this.children.filter(node => node !== child); this.children.push(child); },
  });
  return elements.get(id);
};
const tasks = [...html.matchAll(/<article\b([^>]+)>/g)].map(match => {
  const attrs = Object.fromEntries([...match[1].matchAll(/([\w-]+)="([^"]*)"/g)].map(a => [a[1], a[2]]));
  const details = { open: false };
  return { id: attrs.id, hidden: false, dataset: {
    status: attrs['data-status'], module: attrs['data-module'], search: attrs['data-search'],
    epicOrder: attrs['data-epic-order'], executionOrder: attrs['data-execution-order'],
    available: attrs['data-available'],
  }, querySelector() { return details; } };
});
assert.ok(tasks.length > 0);
const taskList = element('task-list');
taskList.children = [...tasks];
element('view-select').value = 'execution';
const document = { documentElement: { dataset: { theme: 'dark' } }, getElementById: element, querySelectorAll: () => tasks };
const script = html.match(/<script>([\s\S]*?)<\/script>/)?.[1];
assert.ok(script, 'Expected the self-contained progress controller');
// Exercise file/privacy contexts where localStorage access is unavailable.
vm.runInNewContext(script, { document, localStorage: { getItem() { throw Error('unavailable'); }, setItem() { throw Error('unavailable'); } } }, { filename: fileURLToPath(new URL('PROGRESS.html', root)) });
const fire = (id, event, value) => {
  if (value !== undefined) element(id).value = value;
  element(id).listeners[event]();
};
const visible = () => tasks.filter(t => !t.hidden);
const state = JSON.parse(readFileSync(new URL('progress/state.json', root), 'utf8'));
assert.equal(tasks.length, state.tasks.length, 'Simulation must exercise every generated task');
const rowIds = () => taskList.children.map(t => t.id);
const executionIds = rowIds();
const positions = new Map(executionIds.map((id, index) => [id, index]));
assert.equal(typeof element('view-select').listeners.change, 'function', 'View selector must switch task order');
for (const task of state.tasks) {
  for (const dependency of task.depends_on) assert.ok(positions.get(dependency) < positions.get(task.id));
}
fire('view-select', 'change', 'epic');
assert.deepEqual(rowIds(), state.tasks.map(t => t.id));
assert.equal(new Set(rowIds()).size, tasks.length, 'Switching must move rows without cloning them');
fire('view-select', 'change', 'execution');
assert.deepEqual(rowIds(), executionIds);

fire('status-filter', 'change', 'ready');
assert.equal(visible().length, state.tasks.filter(t => t.status === 'ready').length);
assert.ok(visible().every(t => t.dataset.status === 'ready'));
fire('status-filter', 'change', '');
fire('module-filter', 'change', 'Currency');
assert.ok(visible().length > 0);
assert.ok(visible().every(t => t.dataset.module === 'Currency'));
fire('expand-toggle', 'click');
assert.ok(visible().every(t => t.querySelector('details').open));
const openIds = visible().map(t => t.id);
fire('view-select', 'change', 'epic');
assert.deepEqual(visible().map(t => t.id), openIds, 'Module filter survives reordering');
assert.ok(visible().every(t => t.querySelector('details').open), 'Expanded details survive reordering');
fire('view-select', 'change', 'execution');
fire('expand-toggle', 'click');
assert.ok(visible().every(t => !t.querySelector('details').open));
fire('module-filter', 'change', '');
fire('search', 'input', 'no-task-should-match-this-unique-value');
assert.equal(visible().length, 0);
assert.equal(element('empty').hidden, false);
assert.equal(element('visible-count').textContent, '0');
fire('search', 'input', '');
assert.equal(visible().length, tasks.length);
fire('theme-toggle', 'click');
assert.equal(document.documentElement.dataset.theme, 'light');
console.log('PASS: execution/epic views, prerequisite order, preserved filters/details, empty state and theme in DOM simulation');
console.log('NOT RUN: actual browser rendering, responsive layout, accessibility tools or OBS');
