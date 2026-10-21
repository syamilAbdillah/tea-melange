#!/usr/bin/env node
// Drive bench/jsfb in headless Chrome over CDP. Each operation is timed from
// the click to the end of a forced layout in the frame that patches the DOM;
// idle time waiting for that frame is excluded.
// Usage: node bench/jsfb/run.mjs [runs=10]
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { spawn } from 'node:child_process';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, extname, join, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const runs = Number(process.argv[2] ?? 10);
const root = resolve(dirname(fileURLToPath(import.meta.url)), '../../_build/default/bench/jsfb/js');
const chrome = process.env.CHROME ?? 'google-chrome';

const server = createServer(async (req, res) => {
  const file = resolve(root, '.' + new URL(req.url, 'http://x').pathname.replace(/\/$/, '/index.html'));
  if (!file.startsWith(root + sep)) return res.writeHead(403).end();
  try {
    const type = { '.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript' }[extname(file)];
    const data = await readFile(file);
    res.writeHead(200, { 'Content-Type': type ?? 'application/octet-stream' }).end(data);
  } catch { res.writeHead(404).end(); }
});
await new Promise(r => server.listen(0, '127.0.0.1', r));
const origin = `http://127.0.0.1:${server.address().port}`;

const profile = mkdtempSync(join(tmpdir(), 'jsfb-chrome-'));
const proc = spawn(chrome, ['--headless=new', '--remote-debugging-port=0', `--user-data-dir=${profile}`,
  '--no-first-run', '--disable-extensions', '--window-size=1280,800', 'about:blank'], { stdio: ['ignore', 'ignore', 'pipe'] });
const wsUrl = await new Promise((ok, fail) => {
  let buf = '';
  proc.stderr.on('data', d => { buf += d; const m = buf.match(/ws:\/\/\S+/); if (m) ok(m[0]); });
  proc.on('exit', () => fail(new Error('chrome exited: ' + buf)));
});
const port = new URL(wsUrl).port;
const target = (await (await fetch(`http://127.0.0.1:${port}/json/list`)).json()).find(t => t.type === 'page');
const ws = new WebSocket(target.webSocketDebuggerUrl);
await new Promise(r => ws.addEventListener('open', r));
let seq = 0; const pending = new Map();
ws.addEventListener('message', e => { const m = JSON.parse(e.data); pending.get(m.id)?.(m); pending.delete(m.id); });
const send = (method, params = {}) => new Promise((ok, fail) => {
  const id = ++seq; pending.set(id, m => m.error ? fail(new Error(m.error.message)) : ok(m.result));
  ws.send(JSON.stringify({ id, method, params }));
});
const evaluate = async expression => {
  const r = await send('Runtime.evaluate', { expression, awaitPromise: true, returnByValue: true });
  if (r.exceptionDetails) throw new Error(JSON.stringify(r.exceptionDetails));
  return r.result.value;
};

const harness = `
window.frame = () => new Promise(r => requestAnimationFrame(() => setTimeout(r, 0)));
window.measure = async (target) => {
  await frame();
  let start = 0;
  requestAnimationFrame(() => { start = performance.now(); });
  const t0 = performance.now();
  document.querySelector(target).click();
  const clickDone = performance.now();
  const [script, end] = await new Promise(r => requestAnimationFrame(() => { const s = performance.now(); document.body.offsetHeight; r([s, performance.now()]); }));
  return [(clickDone - t0) + (end - start), (clickDone - t0) + (script - start)];
};
window.click = async (target) => { document.querySelector(target).click(); await frame(); await frame(); };
`;
const ops = {
  'create 1k':       { setup: ['#clear'], measure: '#run' },
  'replace 1k':      { setup: ['#run'], measure: '#run' },
  'update 10th':     { setup: ['#run', '#update', '#update', '#update'], measure: '#update' },
  'select row':      { setup: ['#run'], measure: 'tbody tr:nth-child(2) td:nth-child(2) a' },
  'swap rows':       { setup: ['#run'], measure: '#swaprows' },
  'remove row':      { setup: ['#run'], measure: 'tbody tr:nth-child(4) td:nth-child(3) a' },
  'create 10k':      { setup: ['#clear'], measure: '#runlots' },
  'append 1k':       { setup: ['#run'], measure: '#add' },
  'clear 1k':        { setup: ['#run'], measure: '#clear' },
};

const median = xs => { const s = [...xs].sort((a, b) => a - b); const h = s.length >> 1; return s.length % 2 ? s[h] : (s[h - 1] + s[h]) / 2; };
await send('Page.enable');
if (process.env.FLOOR) {
  const code = (await readFile(process.env.FLOOR, 'utf8'));
  for (let i = 0; i < 5; i++) {
    await send('Page.navigate', { url: `${origin}/?lazy` });
    await evaluate(`new Promise(r => { const t = () => document.querySelector('#run') ? r() : setTimeout(t, 10); t(); })`);
    console.log(await evaluate(code));
  }
  ws.close(); server.close(); await new Promise(r => { proc.on('exit', r); proc.kill(); }); rmSync(profile, { recursive: true, force: true }); process.exit(0);
}
// PROFILE="create 10k" prints the hottest functions (self time) for one op.
if (process.env.PROFILE) {
  const op = ops[process.env.PROFILE];
  await send('Page.navigate', { url: `${origin}/?${process.env.VARIANT ?? 'lazy'}` });
  await evaluate(`new Promise(r => { const t = () => document.querySelector('#run') ? r() : setTimeout(t, 10); t(); })`);
  await evaluate(harness);
  for (const s of op.setup) await evaluate(`click(${JSON.stringify(s)})`);
  await send('Profiler.enable');
  await send('Profiler.setSamplingInterval', { interval: 50 });
  await send('Profiler.start');
  for (let i = 0; i < 5; i++) { for (const s of op.setup) await evaluate(`click(${JSON.stringify(s)})`); await evaluate(`measure(${JSON.stringify(op.measure)})`); }
  const { profile: cpu } = await send('Profiler.stop');
  const self = new Map(); const dt = cpu.timeDeltas; const byId = new Map(cpu.nodes.map(n => [n.id, n]));
  cpu.samples.forEach((id, i) => { const n = byId.get(id); const k = `${n.callFrame.functionName || '(anon)'} ${n.callFrame.url.split('/').slice(-2).join('/')}:${n.callFrame.lineNumber + 1}`; self.set(k, (self.get(k) ?? 0) + (dt[i] ?? 0)); });
  const total = [...self.values()].reduce((a, b) => a + b, 0);
  for (const [k, v] of [...self].sort((a, b) => b[1] - a[1]).slice(0, 30)) console.log((v / total * 100).toFixed(1).padStart(5) + '%', k);
  ws.close(); server.close(); await new Promise(r => { proc.on('exit', r); proc.kill(); }); rmSync(profile, { recursive: true, force: true }); process.exit(0);
}
const results = {};
for (const variant of ['baseline', 'lazy']) {
  results[variant] = {};
  for (const [name, op] of Object.entries(ops)) {
    const samples = [], scripts = [];
    for (let i = 0; i < runs; i++) {
      // Fresh page per sample, as js-framework-benchmark does.
      await send('Page.navigate', { url: `${origin}/?${variant}` });
      await evaluate(`new Promise(r => { const t = () => document.querySelector('#run') ? r() : setTimeout(t, 10); t(); })`);
      await evaluate(harness);
      for (let w = 0; w < 3 && name.startsWith('create'); w++) await evaluate(`click('#run').then(() => click('#clear'))`);
      for (const s of op.setup) await evaluate(`click(${JSON.stringify(s)})`);
      const [total, script] = await evaluate(`measure(${JSON.stringify(op.measure)})`);
      samples.push(total); scripts.push(script);
    }
    results[variant][name] = median(samples);
    results[variant + ' js'] ??= {};
    results[variant + ' js'][name] = median(scripts);
  }
}
const names = Object.keys(ops);
// total = click to forced layout; js = same, excluding the layout.
const cols = ['baseline', 'baseline js', 'lazy', 'lazy js'];
console.log('op'.padEnd(14), cols.map(c => c.padStart(12)).join(''));
for (const n of names) console.log(n.padEnd(14), cols.map(c => results[c][n].toFixed(1).padStart(12)).join(''));
const sum = v => names.reduce((a, n) => a + results[v][n], 0);
console.log('TOTAL'.padEnd(14), cols.map(c => sum(c).toFixed(1).padStart(12)).join(''));
ws.close(); server.close(); await new Promise(r => { proc.on('exit', r); proc.kill(); }); rmSync(profile, { recursive: true, force: true });
