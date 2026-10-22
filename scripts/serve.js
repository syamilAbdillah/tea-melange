#!/usr/bin/env node
// Serve an entire Melange emission, including its emitted dependency tree.
import { createServer } from 'node:http';
import { readFile, stat } from 'node:fs/promises';
import { dirname, extname, resolve, sep } from 'node:path';
import { pathToFileURL, fileURLToPath } from 'node:url';

const examples = resolve(dirname(fileURLToPath(import.meta.url)), '..', '_build/default/examples');
const types = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.mjs': 'text/javascript', '.json': 'application/json; charset=utf-8', '.css': 'text/css' };

export async function serveExample(example, { port = 0, host = '127.0.0.1' } = {}) {
  if (!['counter', 'todo', 'fetch_routing'].includes(example) && !/^ui\/[a-z][a-z0-9_-]*$/.test(example)) {
    throw new Error('Example must be counter, todo, fetch_routing, or ui/<name>');
  }
  const root = resolve(examples, example, 'js');
  try { await stat(resolve(root, 'index.html')); } catch {
    throw new Error(`Build first: dune build @examples/melange (missing ${example}/js/index.html)`);
  }
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
  await new Promise((accept, reject) => {
    server.once('error', reject);
    server.listen(port, host, () => { server.removeListener('error', reject); accept(); });
  });
  return { server, url: `http://${host}:${server.address().port}/`, root };
}

if (process.argv[1] && import.meta.url === pathToFileURL(resolve(process.argv[1])).href) {
  try {
    const example = process.argv[2] ?? 'counter';
    const { url, root } = await serveExample(example, { port: Number(process.argv[3] ?? 8000) });
    console.log(`${example}: ${url} (root ${root})`);
  } catch (error) {
    console.error(error.message);
    console.error('Usage: node scripts/serve.js [counter|todo|fetch_routing|ui/<name>] [port]');
    process.exitCode = 1;
  }
}
