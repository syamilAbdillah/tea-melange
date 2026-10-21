// Browser scaffolding only; application assertions live in Reason.
import { JSDOM } from 'jsdom';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';
const dom = new JSDOM('<!doctype html><html><head></head><body><div id="app"></div></body></html>', { url: 'https://tea.test/start' });
Object.assign(globalThis, { window: dom.window, document: dom.window.document });
let frames = [];
globalThis.requestAnimationFrame = callback => { frames.push(callback); return frames.length; };
globalThis.teaFlushFrame = () => { const current = frames; frames = []; current.forEach(callback => callback(0)); };
// Count app-owned navigation hooks independently of runtime's stopped guard.
const hooks = new Map();
const add = dom.window.EventTarget.prototype.addEventListener;
const remove = dom.window.EventTarget.prototype.removeEventListener;
function key(target, type) {
  if (target === dom.window && (type === 'popstate' || type === 'tea:navigation')) return type;
  if (target === dom.window.document.body && type === 'click') return 'links';
}
dom.window.EventTarget.prototype.addEventListener = function(type, callback, options) {
  const k = key(this, type);
  if (k) { if (!hooks.has(k)) hooks.set(k, new Set()); hooks.get(k).add(callback); }
  return add.call(this, type, callback, options);
};
dom.window.EventTarget.prototype.removeEventListener = function(type, callback, options) {
  const k = key(this, type);
  if (k) hooks.get(k)?.delete(callback);
  return remove.call(this, type, callback, options);
};
globalThis.teaHookCount = type => hooks.get(type)?.size ?? 0;
try {
  const tests = await import(pathToFileURL(resolve(process.argv[2])).href);
  await tests.completion;
} finally {
  dom.window.close();
}
