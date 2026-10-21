// jsdom bootstrapping only; assertions and benchmarks live in Reason.
import { JSDOM } from 'jsdom';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';
const dom = new JSDOM('<!doctype html><html><head></head><body><div id="app"></div></body></html>', { url: 'https://tea.test/' });
Object.assign(globalThis, { window: dom.window, document: dom.window.document, Event: dom.window.Event });
let frames = [], requests = 0;
globalThis.requestAnimationFrame = callback => { requests++; frames.push(callback); return requests; };
globalThis.teaFrameRequests = () => requests;
globalThis.teaFlushFrame = () => { const current = frames; frames = []; current.forEach(callback => callback(performance.now())); };
await import(pathToFileURL(resolve(process.argv[2])).href);
dom.window.close();
