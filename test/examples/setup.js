// Bootstrapping only: all checks and interactions are in smoke.re.
import { JSDOM } from 'jsdom';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';
import { readFile } from 'node:fs/promises';
const [, , example, entry, test] = process.argv;
const html = await readFile(new URL(`../../examples/${example}/js/index.html`, import.meta.url), 'utf8');
const dom = new JSDOM(html, { url: example === 'fetch_routing' ? 'https://tea.test/' : 'https://tea.test/deep/route' });
Object.assign(globalThis, { window: dom.window, document: dom.window.document, location: dom.window.location, Event: dom.window.Event, teaExample: example });
let frames = [], requests = 0;
globalThis.requestAnimationFrame = callback => { frames.push(callback); return ++requests; };
globalThis.teaFlushFrame = () => { const current = frames; frames = []; current.forEach(callback => callback(performance.now())); };
globalThis.teaFetchCalls = [];
const fixture = await readFile(new URL('../../examples/fetch_routing/js/users.json', import.meta.url), 'utf8');
globalThis.fetch = async url => {
  globalThis.teaFetchCalls.push(String(url));
  return new Response(fixture, { status: 200, headers: { 'Content-Type': 'application/json' } });
};
globalThis.teaBack = () => new Promise((resolve, reject) => {
  const timer = setTimeout(() => reject(new Error('history.back timed out')), 2000);
  dom.window.addEventListener('popstate', event => { clearTimeout(timer); resolve(event); }, { once: true });
  dom.window.history.back();
});
await import(pathToFileURL(resolve(entry)).href);
// Let fetch/text promise continuations reach the runtime before the Reason checks.
await new Promise(resolve => setTimeout(resolve, 0));
const checks = await import(pathToFileURL(resolve(test)).href);
await checks.completed;
dom.window.close();
