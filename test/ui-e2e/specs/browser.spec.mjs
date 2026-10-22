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

  const tabbable = await page.evaluate(() => {
    const { dom } = window.probe;
    const root = document.createElement('section');
    root.id = 'tabbable-fixture';
    root.setAttribute('aria-label', 'Tab order checks');
    document.querySelector('main').append(root);
    const failures = [];
    const check = (label, html, first, last) => {
      root.innerHTML = html;
      const actual = [dom.first_tabbable(undefined, root)?.id ?? null, dom.last_tabbable(undefined, root)?.id ?? null];
      if (actual[0] !== first || actual[1] !== last) failures.push({ label, actual, expected: [first, last] });
    };
    check('empty', '', null, null);
    check('positive then DOM', '<button id="tab-zero">Zero</button><button id="tab-three" tabindex="3">Three</button><button id="tab-one" tabindex="1">One</button><button id="tab-two" tabindex="2">Two</button>', 'tab-one', 'tab-zero');
    check('equal priorities', '<button id="tab-a" tabindex="2">A</button><button id="tab-b" tabindex="2">B</button>', 'tab-a', 'tab-b');
    check('maximum positive', '<button id="tab-zero">Zero</button><button id="tab-max" tabindex="2147483647">Max</button>', 'tab-max', 'tab-zero');
    check('exclusions', '<div hidden><button>Hidden</button></div><div inert><button>Inert</button></div><div style="display:none"><button>Display</button></div><div style="visibility:collapse"><button>Visibility</button></div><fieldset disabled><button>Disabled</button></fieldset><input type="hidden" tabindex="1"><button tabindex="-1">Negative</button>', null, null);
    check('closed details first summary only', '<details><summary id="tab-summary">Summary<button id="tab-nested">Nested</button></summary><summary tabindex="0">Second</summary><button>Content</button></details>', 'tab-summary', 'tab-nested');
    check('open details', '<details open><summary id="tab-summary">Summary</summary><button id="tab-content">Content</button></details>', 'tab-summary', 'tab-content');
    check('editable/media', '<div id="tab-edit" contenteditable>Editable</div><video id="tab-video" controls></video>', 'tab-edit', 'tab-video');
    check('explicit negative media', '<div contenteditable tabindex="-1">Editable</div><video controls tabindex="-1"></video>', null, null);
    check('checked radio', '<input id="tab-a" type="radio" name="tab-group"><input id="tab-b" type="radio" name="tab-group" checked><input id="tab-c" type="radio" name="tab-group">', 'tab-b', 'tab-b');
    check('unchecked radio direction', '<input id="tab-a" type="radio" name="tab-group"><input id="tab-b" type="radio" name="tab-group">', 'tab-a', 'tab-b');
    check('different form owners', '<form><input id="tab-a" type="radio" name="tab-group"></form><form><input id="tab-b" type="radio" name="tab-group" checked></form>', 'tab-a', 'tab-b');
    root.innerHTML = '<input type="radio" name="tab-group" checked><div id="tab-radio-root"><input type="radio" name="tab-group"></div>';
    const radioRoot = root.querySelector('#tab-radio-root');
    if (dom.first_tabbable(undefined, radioRoot) !== undefined || dom.last_tabbable(undefined, radioRoot) !== undefined) failures.push('checked peer outside root');
    root.innerHTML = '<span data-boundary tabindex="0"></span><button id="tab-two" tabindex="2">Two</button><button id="tab-one" tabindex="1">One</button><span data-boundary tabindex="0"></span>';
    const exclude = node => node.hasAttribute('data-boundary');
    if (dom.first_tabbable(exclude, root)?.id !== 'tab-one' || dom.last_tabbable(exclude, root)?.id !== 'tab-two') failures.push('excluded boundaries');
    const active = document.activeElement;
    dom.first_tabbable(undefined, root);
    dom.last_tabbable(undefined, root);
    if (document.activeElement !== active) failures.push('selection moved focus');
    root.remove();
    return failures;
  });
  assert.deepEqual(tabbable, [], 'shared synchronous first/last tabbable rules');

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
