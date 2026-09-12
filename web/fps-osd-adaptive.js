(()=>{
'use strict';
const q=id=>document.getElementById(id), IDS=['osd1','osd2','osd3','osd4'];
const countKey='fpsOsdMessageCount', destKey='fpsWarningDestination', legacyDestKey='fpsLegacyTempDestination', defaultsKey='fpsAdaptiveDefaultsV4', DEMO='freeclinkerDemoMode', CFG='freeclinkerDemoConfig';
function setv(id,v){const e=q(id);if(!e)return;e.value=v;e.dispatchEvent(new Event('input',{bubbles:true}));e.dispatchEvent(new Event('change',{bubbles:true}))}
function setc(id,v){const e=q(id);if(!e)return;e.checked=!!v;e.dispatchEvent(new Event('change',{bubbles:true}))}
function demo(){return localStorage.getItem(DEMO)==='1'}
function readCfg(){try{return JSON.parse(localStorage.getItem(CFG)||'{}')}catch{return{}}}
function writeCfg(c){localStorage.setItem(CFG,JSON.stringify(c))}
function confirmedDefaults(){
 setv('osd1','{batt}');setv('osd2','{state} {recdur}');setv('osd3','{mode} {res} {fps} {eis}');setv('osd4','{rectf} {rcap}');
 setv('pilotTpl','{stateonly} {batt} {rectf}');setv('craftTpl','');setc('pilotEn',true);setc('craftEn',false);
 setc('fpvFlash',true);setc('fpvPreArm',true);setv('fpvPreArmText','CLEAN LENS');setv('fpvCustomDurationSec','1.0');setc('fpvLowBatt',true);setv('fpvLowPct','10');setv('fpvRecLowMin','5');
 setv('cameraMatch','1');setc('wakeGuard',true);setc('stopOnDisarm',true);setc('debugBle',false);setc('lowPower',true);setc('wifiApEnabled',false);setv('disarmDelay','5');
 localStorage.setItem(countKey,'4');localStorage.setItem(destKey,'1');localStorage.setItem(legacyDestKey,'pilot');localStorage.setItem('fpsCustomMessageText','CLEAN LENS');
 const ce=q('fpsCustomEnable');if(ce){ce.checked=true;ce.dispatchEvent(new Event('change',{bubbles:true}))}
}
function init(){
 const master=q('fpsOsdEnable');if(!master)return;
 document.querySelectorAll('.fps-custom-text-check').forEach(x=>x.closest('label')?.remove());
 document.querySelectorAll('.fps-custom-text-row,.fps-advanced').forEach(x=>x.remove());
 document.querySelectorAll('.token-help,.available-tokens').forEach(x=>x.classList.add('fps-hidden'));
 document.querySelectorAll('.fps-chip').forEach(ch=>{if(ch.firstChild?.nodeType===3&&ch.firstChild.textContent.trim()==='{stateonly}')ch.firstChild.textContent='Status '});
 const custom=q('fpsCustomEnable'),customHead=custom?.closest('.fps-feature-head');
 if(customHead){const n=customHead.querySelector('.cfg-field-name'),d=customHead.querySelector('.cfg-field-desc');if(n)n.textContent='Temporary Message';if(d)d.textContent='Optional message shown temporarily, with first-arm and display-duration controls.'}
 const currentCard=q('osd1')?.closest('.config-card');
 let count=Math.max(1,Math.min(4,parseInt(localStorage.getItem(countKey)||'4',10)||4));
 const controls=document.createElement('div');controls.className='fps-message-count-controls';controls.innerHTML='<button type="button" id="fpsAddOsdMessage">+ Add another OSD message</button><button type="button" id="fpsRemoveOsdMessage">Remove last message</button>';
 currentCard?.querySelector('.card-footer')?.before(controls);
 const dest=q('fpsWarningDestination'),destRow=dest?.closest('.fps-warning-destination');
 function legacyEnabled(){const a=[];if(q('pilotEn')?.checked)a.push(['pilot','Pilot Name']);if(q('craftEn')?.checked)a.push(['craft','Craft Name']);return a}
 function updateDestination(){if(!dest||!destRow)return;const legacy=!!q('bf45Compat')?.checked;if(legacy){const a=legacyEnabled();dest.innerHTML='';a.forEach(([v,n])=>dest.add(new Option(n,v)));const saved=localStorage.getItem(legacyDestKey);if(saved&&a.some(x=>x[0]===saved))dest.value=saved;destRow.classList.toggle('fps-auto-destination',a.length<=1);dest.disabled=a.length<=1;if(a.length===1)dest.value=a[0][0];destRow.querySelector('.cfg-field-name').textContent='Temporary Message destination';destRow.querySelector('.cfg-field-desc').textContent=a.length>1?'Both Pilot Name and Craft Name are enabled. Choose where temporary messages and warnings appear.':a.length===1?`Automatically uses ${a[0][1]}.`:'Enable Pilot Name or Craft Name to choose a destination.';}else{dest.innerHTML='';for(let i=1;i<=count;i++)dest.add(new Option('Custom Message '+i,String(i)));const saved=localStorage.getItem(destKey)||'1';dest.value=String(Math.min(count,Math.max(1,parseInt(saved,10)||1)));destRow.classList.toggle('fps-auto-destination',count===1);dest.disabled=count===1;destRow.querySelector('.cfg-field-name').textContent='Temporary Message destination';destRow.querySelector('.cfg-field-desc').textContent=count>1?'Choose which configured Custom Message slot is temporarily replaced.':'Automatically uses Custom Message 1.';}}
 function applyCount(){IDS.forEach((id,i)=>q(id)?.closest('.fps-builder')?.classList.toggle('fps-user-hidden',i>=count));localStorage.setItem(countKey,String(count));q('fpsAddOsdMessage').disabled=count>=4;q('fpsRemoveOsdMessage').disabled=count<=1;updateDestination()}
 const fields={osd1:'osd1',osd2:'osd2',osd3:'osd3',osd4:'osd4',pilotTpl:'pilotTpl',craftTpl:'craftTpl',bf45Compat:'bf45Compat',pilotEn:'pilotEn',craftEn:'craftEn',fpvFlash:'fpvFlash',fpvPreArm:'fpvPreArm',fpvPreArmText:'fpvPreArmText',fpvCustomDurationSec:'fpvCustomDurationSec',fpvLowBatt:'fpvLowBatt',fpvLowPct:'fpvLowPct',fpvRecLowMin:'fpvRecLowMin',cameraMatch:'cameraMatch',wakeGuard:'wakeGuard',stopOnDisarm:'stopOnDisarm',debugBle:'debugBle',lowPower:'lowPower',wifiApEnabled:'wifiApEnabled',disarmDelay:'disarmDelay'};
 function snapshot(){if(!demo())return;const c=readCfg();for(const[id,key]of Object.entries(fields)){const e=q(id);if(!e)continue;c[key]=e.type==='checkbox'?e.checked:e.value}c.fpsCustomEnable=!!q('fpsCustomEnable')?.checked;c.fpsOsdEnabled=!!master.checked;c.fpsOsdMessageCount=count;c.fpsWarningDestination=localStorage.getItem(destKey)||'1';c.fpsLegacyTempDestination=localStorage.getItem(legacyDestKey)||'pilot';writeCfg(c)}
 function restore(){if(!demo())return false;const c=readCfg();if(!Object.keys(c).length)return false;for(const[id,key]of Object.entries(fields)){const e=q(id);if(!e||c[key]===undefined)continue;if(e.type==='checkbox')e.checked=!!c[key];else e.value=c[key]}if(c.fpsCustomEnable!==undefined&&q('fpsCustomEnable'))q('fpsCustomEnable').checked=!!c.fpsCustomEnable;if(c.fpsOsdEnabled!==undefined)master.checked=!!c.fpsOsdEnabled;if(c.fpsOsdMessageCount!==undefined)count=Math.max(1,Math.min(4,Number(c.fpsOsdMessageCount)||4));if(c.fpsWarningDestination)localStorage.setItem(destKey,String(c.fpsWarningDestination));if(c.fpsLegacyTempDestination)localStorage.setItem(legacyDestKey,String(c.fpsLegacyTempDestination));for(const id of ['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'])q(id)?.dispatchEvent(new Event('input',{bubbles:true}));for(const id of ['bf45Compat','pilotEn','craftEn','fpsCustomEnable','fpvLowBatt','fpsOsdEnable'])q(id)?.dispatchEvent(new Event('change',{bubbles:true}));return true}
 q('fpsAddOsdMessage')?.addEventListener('click',()=>{if(count<4){count++;applyCount();snapshot()}});q('fpsRemoveOsdMessage')?.addEventListener('click',()=>{if(count>1){count--;applyCount();snapshot()}});
 dest?.addEventListener('change',()=>{if(q('bf45Compat')?.checked)localStorage.setItem(legacyDestKey,dest.value);else localStorage.setItem(destKey,dest.value);snapshot()});
 ['pilotEn','craftEn','bf45Compat','fpsCustomEnable','fpvLowBatt'].forEach(id=>q(id)?.addEventListener('change',()=>setTimeout(()=>{updateDestination();snapshot()},0)));
 if(!restore()){if(!localStorage.getItem(defaultsKey)){confirmedDefaults();localStorage.setItem(defaultsKey,'1');count=4}if(demo())snapshot()}
 document.querySelector('#panel-config')?.addEventListener('input',()=>setTimeout(snapshot,0));document.querySelector('#panel-config')?.addEventListener('change',()=>setTimeout(snapshot,0));
 const reset=q('fpsResetDefaults');if(reset){reset.onclick=()=>{if(confirm('Reset the visible FPSteVe settings to the confirmed defaults? You can review them before Apply.')){confirmedDefaults();count=4;applyCount();snapshot()}}}
 applyCount();updateDestination();snapshot();
}
const st=document.createElement('style');st.textContent='.fps-user-hidden{display:none!important}.fps-message-count-controls{display:flex;gap:8px;flex-wrap:wrap;padding:4px 18px 16px}.fps-auto-destination select{display:none!important}.fps-auto-destination .cfg-field-text{max-width:none!important}.fps-builder .fps-advanced{display:none!important}';document.head.appendChild(st);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,260));else setTimeout(init,260);
})();
