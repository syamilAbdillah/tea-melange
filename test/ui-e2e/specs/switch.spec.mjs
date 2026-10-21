import assert from 'node:assert/strict';
import {openExample,frames,pressKeys,assertFocus,ariaSnapshot,axe} from '../lib.mjs';
export async function run(context) {
 const page=await openExample(context,'forms'); const control=page.locator('#updates-control');
 const count=async()=>Number((await page.locator('#activity').textContent()).match(/Toggles: (\d+)/)[1]);
 const snapshot=async(on,disabled=false)=>ariaSnapshot(control,'- switch "Tasting updates"'+(on?' [checked]':'')+(disabled?' [disabled]':'')+': "'+(on?'On':'Off')+'"');
 await snapshot(false); await axe(page);
 await control.focus(); await pressKeys(page,'Enter'); await snapshot(true); await axe(page);
 assert.equal(await count(),1); await assertFocus(page,'#updates-control');
 const scroll=await page.evaluate(()=>scrollY);
 await page.keyboard.down('Space'); await frames(page); assert.equal(await count(),1); assert.equal(await page.evaluate(()=>scrollY),scroll);
 await page.keyboard.up('Space'); await frames(page); assert.equal(await count(),2); assert.equal(await page.evaluate(()=>scrollY),scroll);
 await snapshot(false); await axe(page);
 await page.locator('#updates-label').click(); await frames(page);
 await assertFocus(page,'#updates-control'); assert.equal(await count(),3); await snapshot(true); await axe(page);
 await control.click(); await frames(page); assert.equal(await count(),4); await snapshot(false); await axe(page);
 await page.evaluate(()=>{const el=document.querySelector('#updates-control');el.click();el.click();}); await frames(page);
 assert.equal(await count(),6); await snapshot(false); await axe(page);
 await pressKeys(page,'Tab'); await assertFocus(page,'#tea-control'); await pressKeys(page,'Shift+Tab'); await assertFocus(page,'#updates-control');
 await control.click(); await page.locator('#submit').click(); await frames(page);
 assert.ok(JSON.parse(await page.locator('#serialized').textContent()).some(([k,v])=>k==='updates'&&v==='subscribed'));
 await page.locator('#disable').click(); await frames(page); await snapshot(true,true); await axe(page);
 await control.dispatchEvent('click'); await page.locator('#updates-label').dispatchEvent('click'); await frames(page); assert.equal(await count(),7);
 await page.locator('#submit').click(); await frames(page); assert.deepEqual(JSON.parse(await page.locator('#serialized').textContent()),[]);
 await page.locator('#disable').click(); await frames(page); await snapshot(true); await axe(page);
 await control.click(); await frames(page); await snapshot(false); await axe(page);
 await page.locator('#disable').click(); await frames(page); await snapshot(false,true); await axe(page);
}
