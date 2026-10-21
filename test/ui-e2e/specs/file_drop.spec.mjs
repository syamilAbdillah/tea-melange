import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'file_drop');
  await frames(page);
  const root = page.locator('#files-root');
  const button = page.locator('#files-button');
  const input = page.locator('#files-input');
  const results = () => page.locator('#results').textContent();
  const transfer = async entries => page.evaluateHandle(entries => {
    const data = new DataTransfer();
    for (const { name, body = 'abc', mime = 'text/plain' } of entries) {
      data.items.add(new File([body], name, { type: mime, lastModified: 1700000000123 }));
    }
    return data;
  }, entries);
  const drag = async (selector, type, data) => {
    const allowed = await page.locator(selector).evaluate((node, { type, data }) => node.dispatchEvent(new DragEvent(type, {
      bubbles: true, cancelable: true, dataTransfer: data,
    })), { type, data });
    await frames(page);
    return allowed;
  };
  await ariaSnapshot(button, '- button "Choose files"');
  await ariaSnapshot(page.locator('#camera-button'), '- button "Take a selfie"');
  await ariaSnapshot(page.locator('#rear-button'), '- button "Use rear camera" [disabled]');
  await ariaSnapshot(page.locator('#results'), '- status:\n  - paragraph: No files selected.');
  assert.equal(await input.isHidden(), true);
  assert.equal(await input.getAttribute('multiple'), '');
  assert.equal(await input.getAttribute('accept'), 'image/*,.txt');
  assert.equal(await page.locator('#camera-input').getAttribute('capture'), 'user');
  assert.equal(await page.locator('#rear-input').getAttribute('capture'), 'environment');
  assert.equal(await button.getAttribute('aria-controls'), 'files-input');
  await axe(page);

  // Native Tab/Shift+Tab order skips hidden inputs and disabled buttons.
  await page.locator('#disable').focus();
  await pressKeys(page, 'Tab'); await assertFocus(page, '#files-button');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#camera-button');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#files-button');
  const beforeKeys = await results();
  await pressKeys(page, 'ArrowDown', 'Control+Enter', 'Alt+Enter', 'Meta+Enter');
  assert.equal(await results(), beforeKeys);

  // Real native picker activation is exercised on the user-event stack for
  // pointer, Enter and Space (not an asynchronous synthetic open command).
  for (const activation of ['pointer', 'Enter', 'Space']) {
    await button.focus();
    let chooserCount = 0;
    const count = () => chooserCount++;
    page.on('filechooser', count);
    if (activation === 'Space') {
      await page.keyboard.down('Space'); await frames(page);
      assert.equal(chooserCount, 0, 'Space waits for native keyup');
    }
    const next = page.waitForEvent('filechooser');
    if (activation === 'pointer') await button.click();
    else if (activation === 'Space') await page.keyboard.up('Space');
    else await page.keyboard.press('Enter');
    const chooser = await next;
    assert.equal(chooser.isMultiple(), true);
    await chooser.setFiles({ name: 'same.txt', mimeType: 'text/plain', buffer: Buffer.from('abc') });
    await frames(page);
    page.off('filechooser', count);
    await assertFocus(page, '#files-button');
    assert.equal(await input.inputValue(), '', 'input cleared for repeated same-file selection');
    assert.equal(await page.locator('#results p').count(), ['pointer', 'Enter', 'Space'].indexOf(activation) + 1);
  }
  await axe(page);

  // The HTML accept hint is also validated by update, including chooser paths.
  await input.setInputFiles([
    { name: 'image.PNG', mimeType: 'image/png', buffer: Buffer.from('png') },
    { name: 'blocked.json', mimeType: 'application/json', buffer: Buffer.from('{}') },
    { name: 'huge.txt', mimeType: 'text/plain', buffer: Buffer.alloc(1025) },
    { name: 'notes.TXT', mimeType: '', buffer: Buffer.alloc(1024) },
    { name: 'excess.txt', mimeType: 'text/plain', buffer: Buffer.from('abc') },
  ]);
  await frames(page);
  assert.match(await results(), /Accepted: image.PNG \(3 bytes, image\/png\)/);
  assert.match(await results(), /Accepted: notes.TXT \(1024 bytes, text\/plain\)/);
  assert.match(await results(), /Rejected: blocked.json: type not accepted/);
  assert.match(await results(), /Rejected: huge.txt: file too large/);
  assert.match(await results(), /Rejected: excess.txt: too many files/);
  await ariaSnapshot(page.locator('#results'), `- status:
  - paragraph: "Accepted: same.txt (3 bytes, text/plain)"
  - paragraph: "Accepted: same.txt (3 bytes, text/plain)"
  - paragraph: "Accepted: same.txt (3 bytes, text/plain)"
  - paragraph: "Accepted: image.PNG (3 bytes, image/png)"
  - paragraph: "Accepted: notes.TXT (1024 bytes, text/plain)"
  - paragraph: "Rejected: blocked.json: type not accepted"
  - paragraph: "Rejected: huge.txt: file too large"
  - paragraph: "Rejected: excess.txt: too many files"`);
  await axe(page);

  const data = await transfer([{ name: 'drop.txt' }, { name: 'drop.png', mime: 'image/png' }]);
  assert.equal(await drag('#disable', 'dragover', data), true, 'outside target is not globally blocked');
  const idleSnapshot = await root.ariaSnapshot();
  assert.equal(await drag('#files-root', 'dragenter', data), false);
  assert.equal(await root.getAttribute('data-dragging-over'), '');
  await ariaSnapshot(button, '- button "Choose files"');
  assert.equal(await root.ariaSnapshot(), idleSnapshot, 'drag feedback does not invent ARIA interaction semantics');
  await axe(page);
  assert.equal(await drag('#files-root .drop-detail', 'dragenter', data), false);
  assert.equal(await drag('#files-root .drop-detail', 'dragleave', data), true);
  assert.equal(await root.getAttribute('data-dragging-over'), '', 'nested leave retains outer enter');
  assert.equal(await drag('#files-root', 'dragover', data), false, 'file dragover permits drop');
  assert.equal(await drag('#files-root', 'dragleave', data), true);
  assert.equal(await root.getAttribute('data-dragging-over'), null);
  // Drop must work without prior dragenter, and reset every nested counter.
  assert.equal(await drag('#files-root .drop-detail', 'drop', data), false);
  await drag('#files-root', 'dragenter', data);
  await drag('#files-root .drop-detail', 'dragenter', data);
  assert.equal(await drag('#files-root', 'drop', data), false);
  assert.equal(await root.getAttribute('data-dragging-over'), null);
  assert.match(await results(), /Accepted: drop.txt/);
  assert.match(await results(), /Accepted: drop.png/);
  await axe(page);
  await data.dispose();

  const text = await page.evaluateHandle(() => { const data = new DataTransfer(); data.setData('text/plain', 'not files'); return data; });
  const beforeText = await results();
  for (const event of ['dragenter', 'dragover', 'dragleave', 'drop']) assert.equal(await drag('#files-root', event, text), true);
  assert.equal(await root.getAttribute('data-dragging-over'), null);
  assert.equal(await results(), beforeText);
  await text.dispose();
  const beforeEmpty = await results();
  await input.setInputFiles([]); await frames(page);
  assert.equal(await results(), beforeEmpty, 'cancelled/empty chooser produces no outcome');
  const pasteAllowed = await root.evaluate(node => {
    const clipboard = new DataTransfer(); clipboard.items.add(new File(['x'], 'paste.txt', { type: 'text/plain' }));
    return node.dispatchEvent(new ClipboardEvent('paste', { bubbles: true, cancelable: true, clipboardData: clipboard }));
  });
  assert.equal(pasteAllowed, true); assert.equal(await results(), beforeEmpty);

  await page.locator('#camera-input').setInputFiles({ name: 'selfie.jpg', mimeType: 'image/jpeg', buffer: Buffer.from('jpg') });
  await frames(page);
  assert.match(await results(), /Accepted: selfie.jpg/);
  await axe(page);
  const files = await transfer([{ name: 'disabled.txt' }]);
  await drag('#files-root', 'dragenter', files);
  await page.locator('#disable').check(); await frames(page);
  assert.equal(await root.getAttribute('data-dragging-over'), null);
  assert.equal(await root.getAttribute('data-disabled'), '');
  await ariaSnapshot(button, '- button "Choose files" [disabled]');
  await axe(page);
  const disabledBefore = await results();
  let disabledChoosers = 0;
  const disabledChooser = () => disabledChoosers++;
  page.on('filechooser', disabledChooser);
  for (const event of ['dragenter', 'dragover', 'drop', 'dragleave']) assert.equal(await drag('#files-root', event, files), true);
  await button.dispatchEvent('click');
  await button.dispatchEvent('keydown', { key: 'Enter' });
  await button.dispatchEvent('keydown', { key: ' ' });
  await button.dispatchEvent('keyup', { key: ' ' });
  const disabledBox = await button.boundingBox();
  await page.mouse.click(disabledBox.x + disabledBox.width / 2, disabledBox.y + disabledBox.height / 2);
  await frames(page);
  assert.equal(await results(), disabledBefore);
  page.off('filechooser', disabledChooser);
  assert.equal(disabledChoosers, 0, 'disabled button never opens picker');
  await page.locator('#disable').focus(); await pressKeys(page, 'Tab'); await assertFocus(page, '#camera-button');
  await page.locator('#disable').uncheck(); await frames(page);
  assert.equal(await root.getAttribute('data-disabled'), null);
  assert.equal(await drag('#files-root', 'drop', files), false);
  assert.match(await results(), /Accepted: disabled.txt/);
  await ariaSnapshot(button, '- button "Choose files"');
  await axe(page);
  await files.dispose();
}
