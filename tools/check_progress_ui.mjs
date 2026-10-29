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
    value: '', hidden: false, textContent: '', listeners: {},
    addEventListener(event, callback) { this.listeners[event] = callback; },
  });
  return elements.get(id);
};
const tasks = [...html.matchAll(/<article id="([^"]+)" class="task" data-status="([^"]+)" data-module="([^"]+)" data-search="([^"]*)">/g)].map(match => {
  const details = { open: false };
  return { id: match[1], hidden: false, dataset: { status: match[2], module: match[3], search: match[4] }, querySelector() { return details; } };
});
assert.ok(tasks.length > 0);
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

fire('status-filter', 'change', 'ready');
assert.equal(visible().length, state.tasks.filter(t => t.status === 'ready').length);
assert.ok(visible().every(t => t.dataset.status === 'ready'));
fire('status-filter', 'change', '');
fire('module-filter', 'change', 'Currency');
assert.ok(visible().length > 0);
assert.ok(visible().every(t => t.dataset.module === 'Currency'));
fire('expand-toggle', 'click');
assert.ok(visible().every(t => t.querySelector('details').open));
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
console.log('PASS: offline controller filtering, empty state, expand/collapse and theme in DOM simulation');
console.log('NOT RUN: actual browser rendering, responsive layout, accessibility tools or OBS');
