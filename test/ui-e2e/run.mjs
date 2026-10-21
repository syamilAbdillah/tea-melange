#!/usr/bin/env node
import { readdir } from 'node:fs/promises';
import { dirname, resolve, basename } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { chromium } from 'playwright-core';
import { assertHealthy, closeExamples } from './lib.mjs';

const directory = resolve(dirname(fileURLToPath(import.meta.url)), 'specs');
const selection = process.argv[2] ?? process.env.TEA_E2E_SPEC;
const specs = (await readdir(directory)).filter(name => name.endsWith('.spec.mjs') && (!selection || [name, name.replace('.spec.mjs', '')].includes(basename(selection)))).sort();
if (!specs.length) throw new Error('No e2e specs matched: ' + (selection ?? directory));
let browser;
try {
  browser = await chromium.launch({
    headless: true,
    ...(process.env.TEA_E2E_CHROMIUM ? { executablePath: process.env.TEA_E2E_CHROMIUM } : {}),
  });
  for (const name of specs) {
    const context = await browser.newContext({ viewport: { width: 1100, height: 800 }, reducedMotion: 'reduce' });
    context.setDefaultTimeout(10000);
    try {
      const spec = await import(pathToFileURL(resolve(directory, name)).href);
      if (typeof spec.run !== 'function') throw new Error(name + ' must export async run(context)');
      let timer;
      try {
        await Promise.race([
          spec.run(context),
          new Promise((_, reject) => { timer = setTimeout(() => reject(new Error(name + ' timed out')), 60000); }),
        ]);
      } finally { clearTimeout(timer); }
      assertHealthy(context.pages());
      console.log('PASS ' + name);
    } finally { await context.close(); }
  }
  console.log('e2e: ' + specs.length + ' spec(s) passed');
} finally {
  await browser?.close();
  await closeExamples();
}
