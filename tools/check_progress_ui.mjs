// SPDX-License-Identifier: GPL-3.0-or-later
// Optional Node-only DOM simulation. Does NOT establish real-browser/OBS support.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import vm from 'node:vm';

const root = new URL('../', import.meta.url);
const html = readFileSync(new URL('PROGRESS.html', root), 'utf8');
const attrs = text => Object.fromEntries([...text.matchAll(/([\w-]+)="([^"]*)"/g)].map(a => [a[1], a[2]]));
const elements = new Map();
const element = id => {
  if (!elements.has(id)) elements.set(id, {
    value: '', checked: false, hidden: false, open: false, textContent: '', listeners: {}, children: [],
    addEventListener(event, callback) { this.listeners[event] = callback; },
    appendChild(child) { this.children = this.children.filter(node => node !== child); this.children.push(child); },
  });
  return elements.get(id);
};
const tasks = [...html.matchAll(/<article\b([^>]+)>/g)].map(match => {
  const a = attrs(match[1]);
  const details = { open: false };
  return { id: a.id, hidden: false, dataset: {
    status: a['data-status'], module: a['data-module'], search: a['data-search'],
    epicOrder: a['data-epic-order'], executionOrder: a['data-execution-order'],
    available: a['data-available'],
  }, querySelector() { return details; } };
});
assert.ok(tasks.length > 0);
const filters = ['status', 'module'].map(key => {
  const group = element(`${key}-filter`);
  group.dataset = { key, label: key === 'status' ? 'states' : 'modules' };
  const choices = [...html.matchAll(/<input\b([^>]+)>/g)].map(m => ({ ...attrs(m[1]), checked: /(?:^|\s)checked(?:\s|$)/.test(m[1]) }))
    .filter(a => a.name === key && a.type === 'checkbox').map(a => ({
      value: a.value, checked: a.checked,
      dataset: { label: a['data-label'] }, listeners: {},
      addEventListener(event, callback) { this.listeners[event] = callback; },
    }));
  assert.ok(choices.length > 0, `${key} filter must offer checkbox choices`);
  const actions = ['all', 'none'].map(action => Object.assign(element(`${key}-${action}`), { dataset: { selection: action } }));
  group.querySelectorAll = selector => selector === 'input[type="checkbox"]' ? choices : actions;
  group.querySelector = () => element(`${key}-summary`);
  group.contains = node => node === group || choices.includes(node) || actions.includes(node);
  return { group, choices, actions };
});
const taskList = element('task-list');
taskList.children = [...tasks];
element('view-select').value = 'execution';
const document = {
  documentElement: { dataset: { theme: 'dark' } }, getElementById: element,
  querySelectorAll: selector => selector === '.task' ? tasks : filters.map(f => f.group),
  listeners: {}, addEventListener(event, callback) { this.listeners[event] = callback; },
};
const script = html.match(/<script>([\s\S]*?)<\/script>/)?.[1];
assert.ok(script, 'Expected the self-contained progress controller');
// Exercise file/privacy contexts where localStorage access is unavailable.
vm.runInNewContext(script, { document, localStorage: { getItem() { throw Error('unavailable'); }, setItem() { throw Error('unavailable'); } } }, { filename: fileURLToPath(new URL('PROGRESS.html', root)) });
const fire = (id, event, value) => {
  if (value !== undefined) element(id).value = value;
  element(id).listeners[event]();
};
const choose = (key, values) => {
  const filter = filters.find(f => f.group.dataset.key === key);
  filter.choices.forEach(choice => { choice.checked = values.includes(choice.value); });
  filter.choices[0].listeners.change();
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
assert.equal(visible().length, tasks.length, 'All choices initially selected');

choose('status', ['ready']);
assert.equal(visible().length, state.tasks.filter(t => t.status === 'ready').length);
assert.ok(visible().every(t => t.dataset.status === 'ready'));
choose('status', ['planned', 'ready']);
choose('module', ['Core', 'Currency']);
const expected = state.tasks.filter(t => ['planned', 'ready'].includes(t.status) && ['Core', 'Currency'].includes(t.module)).map(t => t.id).sort();
assert.deepEqual(visible().map(t => t.id).sort(), expected, 'OR within each filter, AND across filters');
assert.equal(element('status-summary').textContent, '2 states');
assert.equal(element('module-summary').textContent, '2 modules');

choose('module', ['Currency']);
assert.equal(element('module-summary').textContent, 'Currency');
fire('search', 'input', 'ratestest-008');
assert.deepEqual(visible().map(t => t.id), ['RATESTEST-008'], 'Search combines with both selections');
fire('expand-toggle', 'click');
assert.ok(visible().every(t => t.querySelector('details').open));
const openIds = visible().map(t => t.id);
fire('view-select', 'change', 'epic');
assert.deepEqual(visible().map(t => t.id), openIds, 'Selections and search survive reordering');
assert.ok(visible().every(t => t.querySelector('details').open), 'Expanded details survive reordering');
fire('view-select', 'change', 'execution');
fire('expand-toggle', 'click');
assert.ok(visible().every(t => !t.querySelector('details').open));
fire('search', 'input', '');
fire('status-none', 'click');
assert.equal(visible().length, 0);
assert.equal(element('status-summary').textContent, 'No states');
assert.equal(element('empty').hidden, false);
fire('status-all', 'click');
assert.ok(visible().length > 0 && visible().every(t => t.dataset.module === 'Currency'));
fire('module-none', 'click');
assert.equal(visible().length, 0);
assert.equal(element('module-summary').textContent, 'No modules');
fire('module-all', 'click');
assert.equal(visible().length, tasks.length);
assert.equal(element('status-summary').textContent, 'All states');
assert.equal(element('module-summary').textContent, 'All modules');
fire('search', 'input', 'no-task-should-match-this-unique-value');
assert.equal(visible().length, 0);
assert.equal(element('empty').hidden, false);
assert.equal(element('visible-count').textContent, '0');
fire('search', 'input', '');
assert.equal(visible().length, tasks.length);
fire('theme-toggle', 'click');
assert.equal(document.documentElement.dataset.theme, 'light');
console.log('PASS: execution/epic views, checkbox state/module selection, combined filters/search, All/None, retained details, empty state and theme in DOM simulation');
console.log('NOT RUN: actual browser rendering, responsive layout, accessibility tools or OBS');
