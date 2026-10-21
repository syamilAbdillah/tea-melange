#!/usr/bin/env node
// Serve an entire Melange emission, not just the directory containing main.mjs.
import { createServer } from 'node:http';
import { readFile, stat } from 'node:fs/promises';
import { dirname, extname, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const example = process.argv[2] ?? 'counter';
if (!['counter', 'todo', 'fetch_routing'].includes(example)) {
  console.error('Usage: node scripts/serve.js [counter|todo|fetch_routing] [port]');
  process.exit(1);
}
const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '_build/default/examples', example, 'js');
try { await stat(resolve(root, 'index.html')); } catch {
  console.error('Build first: dune build @examples/melange');
  process.exit(1);
}
const types = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.mjs': 'text/javascript', '.json': 'application/json; charset=utf-8', '.css': 'text/css' };
const server = createServer(async (req, res) => {
  if (req.method !== 'GET' && req.method !== 'HEAD') {
    res.writeHead(405, { Allow: 'GET, HEAD' }).end(); return;
  }
  try {
    const pathname = decodeURIComponent(new URL(req.url, 'http://localhost').pathname);
    let file = resolve(root, '.' + pathname);
    if (file !== root && !file.startsWith(root + sep)) { res.writeHead(403).end(); return; }
    if (pathname.endsWith('/')) file = resolve(file, 'index.html');
    let data;
    try { data = await readFile(file); } catch (error) {
      // Only extensionless routes are SPA paths; never mask missing modules.
      if (!['ENOENT', 'ENOTDIR'].includes(error.code) || extname(pathname)) throw error;
      file = resolve(root, 'index.html');
      data = await readFile(file);
    }
    res.writeHead(200, { 'Content-Type': types[extname(file)] ?? 'application/octet-stream', 'Cache-Control': 'no-store' });
    res.end(req.method === 'HEAD' ? undefined : data);
  } catch (error) {
    res.writeHead(error instanceof URIError ? 400 : 404).end();
  }
});
server.listen(Number(process.argv[3] ?? 8000), '127.0.0.1', () => {
  console.log(`${example}: http://127.0.0.1:${server.address().port} (root ${root})`);
});
