// Unprofiled fresh jsdom process: cold mount timing is in Reason.
import { JSDOM } from 'jsdom';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';
const dom = new JSDOM('<!doctype html><div id="app"></div>', { url: 'https://tea.test/' });
Object.assign(globalThis, { window: dom.window, document: dom.window.document });
try { await import(pathToFileURL(resolve(process.argv[2])).href); }
finally { dom.window.close(); }
