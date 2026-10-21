// Real jsdom + deterministic frame queue; benchmark/assertions are Reason.
import { JSDOM } from 'jsdom';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';
const dom = new JSDOM('<!doctype html><div id="app"></div>', { url: 'https://tea.test/' });
Object.assign(globalThis, { window: dom.window, document: dom.window.document });
// Observe actual DOM registrations, independently of Tea_dom.Debug counters.
const listeners = new WeakMap();
const proto = dom.window.EventTarget.prototype;
const add = proto.addEventListener, remove = proto.removeEventListener;
proto.addEventListener = function(type, callback, options) {
  const capture = typeof options === 'boolean' ? options : !!options?.capture;
  let entries = listeners.get(this);
  if (!entries) listeners.set(this, entries = []);
  if (!entries.some(e => e.type === type && e.callback === callback && e.capture === capture))
    entries.push({ type, callback, capture });
  return add.call(this, type, callback, options);
};
proto.removeEventListener = function(type, callback, options) {
  const capture = typeof options === 'boolean' ? options : !!options?.capture;
  const entries = listeners.get(this) || [];
  const i = entries.findIndex(e => e.type === type && e.callback === callback && e.capture === capture);
  if (i !== -1) entries.splice(i, 1);
  return remove.call(this, type, callback, options);
};
// Benchmark uses persistent click listeners, not once/AbortSignal listeners.
globalThis.teaListenerCount = node => (listeners.get(node) || []).length;
let frames = [], requests = 0;
globalThis.requestAnimationFrame = callback => { requests++; frames.push(callback); return requests; };
globalThis.teaFrameRequests = () => requests;
globalThis.teaFlushFrame = () => { const current = frames; frames = []; current.forEach(callback => callback(performance.now())); };
try { await import(pathToFileURL(resolve(process.argv[2])).href); }
finally { dom.window.close(); }
