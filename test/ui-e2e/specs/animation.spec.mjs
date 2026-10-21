import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

const titles = {
  transition: 'CSS transition', keyframe: 'CSS keyframes', none: 'No animation CSS',
  appear: 'Animate initial mount', initial: 'Initially open, without appear',
  unmount: 'Unmount after Left',
};
const panelSnapshot = id => `- region "${titles[id]} content":
  - paragraph: The panel stays mounted while leaving.
  - button "Close panel"`;
const state = (page, id, phase) => page.waitForFunction(([id, phase]) =>
  document.getElementById(id).dataset.phase === phase, [id, phase]);
const counts = (page, id) => page.locator('#' + id).evaluate(el => [Number(el.dataset.entered), Number(el.dataset.left)]);

async function record(page, id) {
  await page.evaluate(id => {
    window.animationRecording?.stop();
    const root = document.getElementById(id), panel = document.getElementById(id + '-panel');
    let frame = 0, live = true;
    const samples = [], ends = [];
    const sample = () => {
      const style = getComputedStyle(panel);
      samples.push({ frame, time: performance.now(), phase: root.dataset.phase,
        closed: panel.hasAttribute('data-closed'), enter: panel.hasAttribute('data-enter'),
        leave: panel.hasAttribute('data-leave'), transition: panel.hasAttribute('data-transition'),
        hidden: panel.hidden, opacity: Number(style.opacity),
        entered: Number(root.dataset.entered), left: Number(root.dataset.left) });
    };
    const observer = new MutationObserver(sample);
    observer.observe(root, { attributes: true, subtree: true, childList: true });
    const end = event => { if (event.target === panel) ends.push({time: performance.now(), opacity: Number(getComputedStyle(panel).opacity), type: event.type}); };
    panel.addEventListener('transitionend', end); panel.addEventListener('animationend', end);
    const loop = () => { if (live) { frame++; sample(); requestAnimationFrame(loop); } };
    requestAnimationFrame(loop); sample();
    window.animationRecording = {samples, ends, stop() {
      live = false; observer.disconnect();
      panel.removeEventListener('transitionend', end); panel.removeEventListener('animationend', end);
    }};
  }, id);
}
async function take(page) {
  return page.evaluate(() => { const r = window.animationRecording; r.stop(); return {samples: r.samples, ends: r.ends}; });
}
async function pauseMidway(page, id) {
  await page.waitForFunction(id => {
    const el = document.getElementById(id + '-panel'), opacity = Number(getComputedStyle(el).opacity);
    if (opacity > .856 && opacity < .994) {
      el.getAnimations({subtree: true}).forEach(animation => animation.pause()); return true;
    }
    return false;
  }, id);
}
const resume = (page, id) => page.locator('#' + id + '-panel').evaluate(el => el.getAnimations({subtree: true}).forEach(animation => animation.play()));

export async function run(context) {
  const page = await openExample(context, 'animation');
  await state(page, 'appear', 'Open');
  assert.deepEqual(await counts(page, 'appear'), [1, 0], 'initial appear completes even with reduced motion');
  assert.deepEqual(await counts(page, 'initial'), [0, 0], 'initial open does not synthesize Entered');
  for (const id of ['transition', 'keyframe', 'none']) {
    await ariaSnapshot(page.locator('#' + id + '-panel'), '');
    await ariaSnapshot(page.locator('#' + id + '-toggle'), `- button "Toggle ${titles[id]}"`);
  }
  await ariaSnapshot(page.locator('#appear-panel'), panelSnapshot('appear'));
  await axe(page);

  await page.emulateMedia({ reducedMotion: 'no-preference' });
  for (const id of ['transition', 'keyframe']) {
    await record(page, id);
    await page.locator('#' + id + '-toggle').click();
    await pauseMidway(page, id);
    assert.equal(await page.locator('#' + id).getAttribute('data-phase'), 'Entering');
    assert.deepEqual(await counts(page, id), [0, 0]);
    assert.equal(await page.locator('#' + id + '-panel').getAttribute('hidden'), null);
    await ariaSnapshot(page.locator('#' + id + '-panel'), panelSnapshot(id));
    await ariaSnapshot(page.locator('#' + id + '-toggle'), `- button "Toggle ${titles[id]}" [expanded]`);
    await axe(page);
    await assertFocus(page, '#' + id + '-toggle'); // Animation never steals focus.
    await resume(page, id);
    await state(page, id, 'Open');
    const enter = await take(page);
    const initial = enter.samples.find(s => s.enter && s.closed && s.transition);
    const active = enter.samples.find(s => s.enter && !s.closed && s.transition);
    const completed = enter.samples.find(s => s.phase === 'Open');
    assert.ok(initial && active && completed, id + ' has both enter attribute stages');
    assert.ok(active.frame > initial.frame, 'initial stage crosses a frame boundary');
    assert.equal(initial.hidden, false);
    assert.ok(enter.samples.some(s => s.enter && s.opacity > .856 && s.opacity < .994), 'real intermediate CSS opacity');
    const end = enter.ends.find(e => e.type === (id === 'transition' ? 'transitionend' : 'animationend'));
    assert.ok(end, 'real CSS completion event');
    assert.ok(end.opacity > .98, 'CSS has reached the visible endpoint before Entered');
    assert.ok(completed.time >= end.time - 34, 'Entered cannot precede CSS finish (two-frame timing tolerance)');
    assert.ok(completed.time - initial.time >= 240, '320ms CSS is actually awaited, allowing startup frame skew');
    assert.ok(enter.samples.filter(s => s.enter && s.opacity < .994).every(s => s.entered === 0), 'no early Entered');
    assert.deepEqual(await counts(page, id), [1, 0]);
    assert.equal(await page.locator('#' + id + '-panel').getAttribute('data-transition'), null);
    await axe(page);

    await record(page, id);
    await page.locator('#' + id + '-close').click();
    await pauseMidway(page, id);
    assert.equal(await page.locator('#' + id).getAttribute('data-phase'), 'Leaving');
    for (const flag of ['closed', 'leave', 'transition']) assert.equal(await page.locator('#' + id + '-panel').getAttribute('data-' + flag), '');
    assert.equal(await page.locator('#' + id + '-panel').getAttribute('hidden'), null);
    await ariaSnapshot(page.locator('#' + id + '-panel'), panelSnapshot(id));
    await assertFocus(page, '#' + id + '-close');
    await axe(page);
    await resume(page, id); await state(page, id, 'Closed');
    const leave = await take(page);
    const leaving = leave.samples.find(s => s.leave), closed = leave.samples.find(s => s.phase === 'Closed');
    const leaveEnd = leave.ends.find(e => e.type === (id === 'transition' ? 'transitionend' : 'animationend'));
    assert.ok(leaving && closed && leaveEnd);
    assert.ok(leaveEnd.opacity < .86, 'leave CSS reaches its endpoint before Left');
    assert.ok(closed.time >= leaveEnd.time - 34);
    assert.ok(closed.time - leaving.time >= 240, 'leave CSS is awaited');
    assert.ok(leave.samples.filter(s => s.leave && s.opacity > .856).every(s => s.left === 0), 'no early Left');
    assert.deepEqual(await counts(page, id), [1, 1]);
    await frames(page); await assertFocus(page, '#' + id + '-toggle');
    await ariaSnapshot(page.locator('#' + id + '-panel'), '');
    await axe(page);
  }

  // Reversals during real CSS, including completion promises for cancelled CSS.
  for (const id of ['transition', 'keyframe']) {
    const before = await counts(page, id);
    await page.locator('#' + id + '-show').click(); await pauseMidway(page, id);
    await page.evaluate(id => new Promise(resolve => {
      document.getElementById(id + '-hide').click();
      const reverse = () => {
        if (document.getElementById(id + '-panel').hasAttribute('data-leave')) {
          document.getElementById(id + '-show').click(); resolve();
        } else requestAnimationFrame(reverse);
      };
      requestAnimationFrame(reverse);
    }), id);
    await state(page, id, 'Open');
    assert.deepEqual(await counts(page, id), [before[0] + 1, before[1]], 'only latest generation emits');
    await page.locator('#' + id + '-hide').click(); await state(page, id, 'Closed');
    const after = await counts(page, id);
    await page.evaluate(id => {
      // No render between these events: helpers must consume intents, not closures.
      for (let i = 0; i < 101; i++) document.getElementById(id + '-toggle').click();
    }, id);
    await state(page, id, 'Open');
    assert.deepEqual(await counts(page, id), [after[0] + 1, after[1]]);
    await page.locator('#' + id + '-hide').click(); await state(page, id, 'Closed');
  }

  // APG-neutral animation: native disclosure controls keep Enter/Space/Tab behavior.
  await page.locator('#none-toggle').focus(); await pressKeys(page, 'Enter'); await state(page, 'none', 'Open');
  await assertFocus(page, '#none-toggle');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#none-show');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#none-toggle');
  await pressKeys(page, 'ArrowDown', 'Escape', 'Control+ArrowDown', 'Alt+ArrowDown', 'Meta+ArrowDown');
  assert.equal(await page.locator('#none-toggle').evaluate(el => el.dispatchEvent(new KeyboardEvent('keydown', {
    key: 'Enter', isComposing: true, bubbles: true, cancelable: true,
  }))), true, 'IME events are not intercepted');
  assert.equal(await page.locator('#none').getAttribute('data-phase'), 'Open');
  await pressKeys(page, 'Space'); await state(page, 'none', 'Closed');
  await ariaSnapshot(page.locator('#none-panel'), '');
  await axe(page);

  await page.emulateMedia({ reducedMotion: 'reduce' });
  for (const id of ['transition', 'keyframe']) {
    await record(page, id); await page.locator('#' + id + '-show').click(); await state(page, id, 'Open');
    const reduced = await take(page);
    assert.ok(reduced.samples.some(s => s.enter && s.closed), 'reduced motion preserves the lifecycle protocol');
    assert.equal(reduced.ends.length, 0, 'CSS motion is removed by the media query');
    assert.equal(await page.locator('#' + id + '-panel').evaluate(el => el.getAnimations().length), 0);
    await ariaSnapshot(page.locator('#' + id + '-panel'), panelSnapshot(id)); await axe(page);
    await page.locator('#' + id + '-hide').click(); await state(page, id, 'Closed');
  }
  await page.locator('#unmount-toggle').click(); await state(page, 'unmount', 'Open');
  await page.locator('#unmount-close').click(); await state(page, 'unmount', 'Closed');
  assert.equal(await page.locator('#unmount-panel').count(), 0);
  await frames(page); await assertFocus(page, '#unmount-toggle');
  assert.equal(await page.locator('#unmount-toggle').getAttribute('aria-controls'), null);

  // Removal while the wait is in flight resolves, even though no CSS end fires.
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  await page.locator('#transition-show').click(); await pauseMidway(page, 'transition');
  await page.locator('#transition-panel').evaluate(el => el.remove());
  await state(page, 'transition', 'Open');
  // Reload to restore the example-owned DOM after intentional external removal.
  await page.reload(); await state(page, 'appear', 'Open');
  assert.deepEqual(await counts(page, 'appear'), [1, 0]);
  assert.deepEqual(await counts(page, 'initial'), [0, 0]);
  await axe(page);
}
