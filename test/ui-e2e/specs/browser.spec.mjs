import assert from 'node:assert/strict';
import { openExample, frames, assertFocus, axe } from '../lib.mjs';

// These checks intentionally call the emitted public Reason APIs. The source
// constructors (Sub.Leaf/Filtered) and leaf lifecycle are public in sub.rei;
// runtime identity reconciliation itself is covered by the jsdom program tests.
export async function run(context) {
  const page = await openExample(context, 'button');
  await page.evaluate(async () => {
    const dom = await import('tea.browser/browser_dom.mjs');
    const events = await import('tea.browser/browser_events.mjs');
    const run = task => new Promise((resolve, reject) => task(result => result.TAG === 0 ? resolve(result._0) : reject(new Error(JSON.stringify(result)))));
    const start = (sub, values = []) => {
      const leaf = sub._0;
      return leaf.start(raw => values.push(leaf.tagger(raw)));
    };
    const fixture = document.createElement('section');
    fixture.id = 'browser-fixture';
    fixture.setAttribute('aria-label', 'Browser effect checks');
    fixture.innerHTML = '<button id="probe-trigger">Open probe</button><div id="probe" style="width:120px;height:40px;background:#273f85"></div><dialog id="probe-dialog" aria-label="Probe dialog"><button id="probe-close">Close probe</button></dialog>';
    document.querySelector('main').append(fixture);
    window.probe = { dom, events, run, start, media: [], sizes: [] };
    window.probe.stopMedia = start(events.on_media_query('(prefers-reduced-motion: reduce)', x => x), window.probe.media);
    window.probe.stopResize = start(events.on_element_resize('probe', x => x), window.probe.sizes);
    await run(dom.focus(undefined, 'probe-trigger'));
  });
  await assertFocus(page, '#probe-trigger');
  await page.waitForFunction(() => window.probe.media.length && window.probe.sizes.length);
  assert.equal(await page.evaluate(() => window.probe.media.at(-1)), true);
  assert.deepEqual(await page.evaluate(() => window.probe.sizes.at(-1)), { width: 120, height: 40 });
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  await page.waitForFunction(() => window.probe.media.at(-1) === false);
  await page.locator('#probe').evaluate(element => { element.style.width = '180px'; });
  await page.waitForFunction(() => window.probe.sizes.at(-1).width === 180);

  const geometry = await page.evaluate(() => window.probe.run(window.probe.dom.get_element('probe')));
  assert.equal(geometry.rect.width, 180);
  assert.equal(geometry.rect.height, 40);
  await page.evaluate(async () => {
    const p = window.probe;
    p.restore = p.start(p.events.restore_focus('probe', undefined, undefined));
    await p.run(p.dom.show_modal('probe-dialog'));
  });
  assert.equal(await page.locator('#probe-dialog').evaluate(element => element.matches(':modal')), true);
  await assertFocus(page, '#probe-close');
  await axe(page);
  await page.evaluate(async () => { const p = window.probe; await p.run(p.dom.close('probe-dialog')); p.restore(); });
  await frames(page);
  await assertFocus(page, '#probe-trigger');
  await page.evaluate(async () => { const p = window.probe; await p.run(p.dom.show('probe-dialog')); });
  assert.equal(await page.locator('#probe-dialog').evaluate(element => element.open && !element.matches(':modal')), true);
  await axe(page);
  await page.evaluate(() => window.probe.run(window.probe.dom.close('probe-dialog')));

  const animations = await page.evaluate(async () => {
    const { dom, run } = window.probe;
    const element = document.querySelector('#probe');
    element.animate([{ opacity: 0 }, { opacity: 1 }], { duration: 80 });
    const before = performance.now();
    await run(dom.wait_for_animations('probe'));
    const elapsed = performance.now() - before;
    const cancelled = element.animate([{ opacity: 0 }, { opacity: 1 }], { duration: 10000 });
    const cancellation = run(dom.wait_for_animations('probe'));
    await new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    cancelled.cancel();
    await cancellation;
    element.animate([{ opacity: 0 }, { opacity: 1 }], { duration: 10000 });
    const removal = run(dom.wait_for_animations('probe'));
    await new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    element.remove();
    await removal;
    return elapsed;
  });
  assert.ok(animations >= 40, 'waits for real animation completion');
  assert.ok(animations < 5000, 'animation settles without watchdog');

  const styles = await page.evaluate(async () => {
    const p = window.probe;
    const before = [document.documentElement, document.body].map(e => e.getAttribute('style'));
    p.lockA = p.start(p.events.scroll_lock('a'));
    p.lockB = p.start(p.events.scroll_lock('b'));
    return before;
  });
  await frames(page);
  assert.equal(await page.evaluate(() => document.documentElement.style.overflow), 'hidden');
  await page.evaluate(() => window.probe.lockA()); await frames(page);
  assert.equal(await page.evaluate(() => document.documentElement.style.overflow), 'hidden');
  await page.evaluate(() => window.probe.lockB()); await frames(page);
  assert.deepEqual(await page.evaluate(() => [document.documentElement, document.body].map(e => e.getAttribute('style'))), styles);
  await page.evaluate(() => { window.probe.stopMedia(); window.probe.stopResize(); document.querySelector('#browser-fixture').remove(); });
  await axe(page);
}
