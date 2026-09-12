(()=>{
'use strict';
const q=id=>document.getElementById(id), IDS=['osd1','osd2','osd3','osd4'];
const countKey='fpsOsdMessageCount', destKey='fpsWarningDestination', legacyDestKey='fpsLegacyTempDestination';
function init(){
 const master=q('fpsOsdEnable'); if(!master)return;
 document.querySelectorAll('.fps-custom-text-check').forEach(x=>x.closest('label')?.remove());
 document.querySelectorAll('.fps-custom-text-row,.fps-advanced').forEach(x=>x.remove());
 document.querySelectorAll('.token-help,.available-tokens').forEach(x=>x.classList.add('fps-hidden'));
 const custom=q('fpsCustomEnable'), customHead=custom?.closest('.fps-feature-head');
 if(customHead){const n=customHead.querySelector('.cfg-field-name'),d=customHead.querySelector('.cfg-field-desc');if(n)n.textContent='Temporary Message';if(d)d.textContent='Optional message shown temporarily, with first-arm and display-duration controls.'}
 const currentCard=q('osd1')?.closest('.config-card');
 let count=Math.max(1,Math.min(4,parseInt(localStorage.getItem(countKey)||'1',10)||1));
 const controls=document.createElement('div'); controls.className='fps-message-count-controls';
 controls.innerHTML='<button type="button" id="fpsAddOsdMessage">+ Add another OSD message</button><button type="button" id="fpsRemoveOsdMessage">Remove last message</button>';
 currentCard?.querySelector('.card-footer')?.before(controls);
 function applyCount(){IDS.forEach((id,i)=>q(id)?.closest('.fps-builder')?.classList.toggle('fps-user-hidden',i>=count));localStorage.setItem(countKey,String(count));q('fpsAddOsdMessage').disabled=count>=4;q('fpsRemoveOsdMessage').disabled=count<=1;updateDestination()}
 q('fpsAddOsdMessage')?.addEventListener('click',()=>{if(count<4){count++;applyCount()}});q('fpsRemoveOsdMessage')?.addEventListener('click',()=>{if(count>1){count--;applyCount()}});
 const dest=q('fpsWarningDestination'), destRow=dest?.closest('.fps-warning-destination');
 function legacyEnabled(){const a=[];if(q('pilotEn')?.checked)a.push(['pilot','Pilot Name']);if(q('craftEn')?.checked)a.push(['craft','Craft Name']);return a}
 function updateDestination(){if(!dest||!destRow)return;const legacy=!!q('bf45Compat')?.checked;if(legacy){const a=legacyEnabled();dest.innerHTML='';a.forEach(([v,n])=>dest.add(new Option(n,v)));const saved=localStorage.getItem(legacyDestKey);if(saved&&a.some(x=>x[0]===saved))dest.value=saved;destRow.classList.toggle('fps-auto-destination',a.length<=1);dest.disabled=a.length<=1;if(a.length===1)dest.value=a[0][0];destRow.querySelector('.cfg-field-name').textContent='Temporary Message destination';destRow.querySelector('.cfg-field-desc').textContent=a.length>1?'Both Pilot Name and Craft Name are enabled. Choose where temporary messages and warnings appear.':a.length===1?`Automatically uses ${a[0][1]}.`:'Enable Pilot Name or Craft Name to choose a destination.';}else{dest.innerHTML='';for(let i=1;i<=count;i++)dest.add(new Option('Custom Message '+i,String(i)));const saved=localStorage.getItem(destKey)||'1';dest.value=String(Math.min(count,Math.max(1,parseInt(saved,10)||1)));destRow.classList.toggle('fps-auto-destination',count===1);dest.disabled=count===1;destRow.querySelector('.cfg-field-name').textContent='Temporary Message destination';destRow.querySelector('.cfg-field-desc').textContent=count>1?'Choose which configured Custom Message slot is temporarily replaced.':'Automatically uses Custom Message 1.';}}
 dest?.addEventListener('change',()=>{if(q('bf45Compat')?.checked)localStorage.setItem(legacyDestKey,dest.value);else localStorage.setItem(destKey,dest.value)});
 ['pilotEn','craftEn','bf45Compat','fpsCustomEnable','fpvLowBatt'].forEach(id=>q(id)?.addEventListener('change',()=>setTimeout(updateDestination,0)));
 applyCount(); updateDestination();
}
const st=document.createElement('style');st.textContent='.fps-user-hidden{display:none!important}.fps-message-count-controls{display:flex;gap:8px;flex-wrap:wrap;padding:4px 18px 16px}.fps-auto-destination select{display:none!important}.fps-auto-destination .cfg-field-text{max-width:none!important}.fps-builder .fps-advanced{display:none!important}';document.head.appendChild(st);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,180));else setTimeout(init,180);
})();