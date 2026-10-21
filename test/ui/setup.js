// Shared jsdom bootstrap. Assertions/interactions live in emitted Reason tests.
import { JSDOM } from 'jsdom';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';

const dom = new JSDOM('<!doctype html><html lang="en"><head><title>UI test</title></head><body><div id="app"></div></body></html>', { url: 'https://tea.test/' });
const { window } = dom;
Object.assign(globalThis, {
  window, document: window.document, Event: window.Event,
  KeyboardEvent: window.KeyboardEvent, MouseEvent: window.MouseEvent,
  PointerEvent: window.PointerEvent ?? class PointerEvent extends window.MouseEvent {
    constructor(type, options = {}) {
      super(type, options);
      Object.defineProperties(this, {
        pointerId: { value: options.pointerId ?? 1 },
        pointerType: { value: options.pointerType ?? 'mouse' },
        isPrimary: { value: options.isPrimary ?? true },
      });
    }
  },
  FocusEvent: window.FocusEvent, MutationObserver: window.MutationObserver,
});
window.PointerEvent = globalThis.PointerEvent;
let frames = [], requests = 0;
const live = new Set();
globalThis.requestAnimationFrame = callback => {
  const id = ++requests; live.add(id); frames.push([id, callback]); return id;
};
globalThis.cancelAnimationFrame = id => live.delete(id);
window.requestAnimationFrame = globalThis.requestAnimationFrame;
window.cancelAnimationFrame = globalThis.cancelAnimationFrame;
globalThis.teaFrameRequests = () => requests;
globalThis.teaPendingFrames = () => live.size;
globalThis.teaFlushFrame = () => {
  const current = frames; frames = [];
  for (const [id, callback] of current) if (live.delete(id)) callback(performance.now());
};
// Prime jsdom's own lazy selector/style listeners before counting library work.
window.document.querySelectorAll(':disabled');
window.getComputedStyle(window.document.body);
await new Promise(resolve => setTimeout(resolve, 0));
const prototype = window.EventTarget.prototype;
const add = prototype.addEventListener, remove = prototype.removeEventListener;
const registrations = [];
const initialStyles = [window.document.documentElement, window.document.body].map(node => node.getAttribute('style'));
const captureOf = options => typeof options === 'boolean' ? options : !!options?.capture;
const forget = record => {
  const index = registrations.indexOf(record);
  if (index >= 0) registrations.splice(index, 1);
  if (record.signal) remove.call(record.signal, 'abort', record.abort);
};
prototype.addEventListener = function (type, callback, options) {
  if ((this !== window.document && this !== window) || !callback) return add.call(this, type, callback, options);
  const capture = captureOf(options);
  const existing = registrations.find(r => r.target === this && r.type === type && r.callback === callback && r.capture === capture);
  if (existing) return add.call(this, type, existing.wrapper, options);
  if (options?.signal?.aborted) return;
  const record = { target: this, type, callback, capture, signal: options?.signal };
  record.wrapper = function (event) {
    if (options?.once) forget(record);
    return typeof callback === 'function' ? callback.call(this, event) : callback.handleEvent(event);
  };
  record.abort = () => forget(record);
  registrations.push(record);
  if (record.signal) add.call(record.signal, 'abort', record.abort, { once: true });
  return add.call(this, type, record.wrapper, options);
};
prototype.removeEventListener = function (type, callback, options) {
  const capture = captureOf(options);
  const record = registrations.find(r => r.target === this && r.type === type && r.callback === callback && r.capture === capture);
  if (record) { forget(record); return remove.call(this, type, record.wrapper, options); }
  return remove.call(this, type, callback, options);
};
globalThis.teaListenerCount = () => registrations.length;
globalThis.teaListenerCountFor = (target, type, capture) =>
  registrations.filter(r => r.target === (target === 'window' ? window : window.document) && r.type === type && r.capture === capture).length;
// A controllable fixture, not an implementation of layout. Missing APIs remain
// missing until a test explicitly installs the mock and restores it afterwards.
globalThis.teaInstallResizeObserverMock = () => {
  const previous = globalThis.ResizeObserver;
  const observers = new Set();
  let disconnected = 0;
  globalThis.ResizeObserver = class {
    constructor(callback) { this.callback = callback; this.targets = []; observers.add(this); }
    observe(target) { this.targets.push(target); }
    disconnect() { disconnected++; observers.delete(this); }
  };
  return {
    notify(width, height) {
      for (const observer of observers) observer.callback(observer.targets.map(target => ({ target, contentRect: { width, height } })));
    },
    count: () => observers.size,
    disconnected: () => disconnected,
    restore: () => { globalThis.ResizeObserver = previous; },
  };
};
globalThis.teaInstallScrollPlatformMock = (platform, maxTouchPoints) => {
  const navigator = Object.getOwnPropertyDescriptor(globalThis, 'navigator');
  const x = Object.getOwnPropertyDescriptor(window, 'scrollX');
  const y = Object.getOwnPropertyDescriptor(window, 'scrollY');
  const scrollTo = window.scrollTo;
  const calls = [];
  Object.defineProperty(globalThis, 'navigator', { configurable: true, value: { platform, maxTouchPoints } });
  Object.defineProperty(window, 'scrollX', { configurable: true, value: 12 });
  Object.defineProperty(window, 'scrollY', { configurable: true, value: 80 });
  window.scrollTo = (x, y) => calls.push([x, y]);
  return {
    calls: () => calls,
    restore() {
      if (navigator) Object.defineProperty(globalThis, 'navigator', navigator); else delete globalThis.navigator;
      Object.defineProperty(window, 'scrollX', x);
      Object.defineProperty(window, 'scrollY', y);
      window.scrollTo = scrollTo;
    },
  };
};
let watchdog;
try {
  const tests = await import(pathToFileURL(resolve(process.argv[2])).href);
  if (!tests.completed || typeof tests.completed.then !== 'function') throw new Error('UI tests must export a completed Promise');
  await Promise.race([
    tests.completed,
    new Promise((_, reject) => { watchdog = setTimeout(() => reject(new Error('UI test timed out')), 10000); }),
  ]);
  globalThis.teaFlushFrame();
  globalThis.teaFlushFrame();
  if (registrations.length) throw new Error('Leaked document/window listeners: ' + registrations.map(r => r.type).join(', '));
  const finalStyles = [window.document.documentElement, window.document.body].map(node => node.getAttribute('style'));
  if (JSON.stringify(initialStyles) !== JSON.stringify(finalStyles)) throw new Error('Leaked document scroll lock/inline styles');
} finally {
  clearTimeout(watchdog);
  prototype.addEventListener = add;
  prototype.removeEventListener = remove;
  dom.window.close();
}
