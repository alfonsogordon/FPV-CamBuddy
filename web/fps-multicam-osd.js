(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const $=id=>document.getElementById(id);
const ADV_KEY='fpsAdvancedMultiCamOsd', ID_KEY='fpsAdvancedMultiCamOsdIdentifier';
const SIMPLE_KEY='fpsMultiCamOsdSimpleTemplatesV1', ADV_TPL_KEY='fpsMultiCamOsdAdvancedTemplatesV1';
const DEMO_KEY='freeclinkerDemoMode', MULTI_KEY='fpsExperimentalMultiCamSync';
const LIMIT=16;
const FIELD_TOKEN={
 'Status':'state','Battery':'batt','Recording Duration':'recdur','Mode':'mode','Resolution':'res',
 'FPS':'fps','Stabilisation':'eis','Time Remaining':'rectf','Storage Remaining':'rcap'
};
const FIELD_NAME=Object.fromEntries(Object.entries(FIELD_TOKEN).map(([k,v])=>[v,k]));
const TEMPLATE_IDS=['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'];
const cameras=new Map();
let registry=[];

const demo=()=>localStorage.getItem(DEMO_KEY)==='1';
function multiOn(){return !!$('fpsMultiCamSync')?.checked||localStorage.getItem(MULTI_KEY)==='1'}
function advancedOn(){return multiOn()&&localStorage.getItem(ADV_KEY)==='1'}
function idMode(){return localStorage.getItem(ID_KEY)==='tag'?'tag':'number'}
function cleanLabel(v){return String(v||'').replace(/[\[\]\x00-\x1f\x7f]/g,'').trim().replace(/\s+/g,' ').slice(0,7)}
function readTemplates(){const out={};for(const id of TEMPLATE_IDS){const e=$(id);if(e)out[id]=e.value||''}return out}
function writeTemplates(map){if(!map)return;for(const id of TEMPLATE_IDS){const e=$(id);if(e&&map[id]!==undefined){e.value=map[id];e.dispatchEvent(new Event('input',{bubbles:true}))}}}
function loadJson(k){try{return JSON.parse(localStorage.getItem(k)||'null')}catch{return null}}
function saveJson(k,v){localStorage.setItem(k,JSON.stringify(v))}
function cameraId(c,mode=idMode()){
 const n=Math.max(1,Number(c?.number)||1),label=cleanLabel(c?.label);
 return mode==='tag'?(label||('C'+n)):('C'+n);
}
function suffix(c,mode=idMode()){return c?'-'+cameraId(c,mode):''}
function parseToken(tok){const m=String(tok||'').match(/^([a-z0-9]+)@(\d+)([nt])$/i);return m?{base:m[1],source:Number(m[2]),mode:m[3]==='t'?'tag':'number'}:null}
function tokenFor(base,source){return `{${base}@${Math.max(0,Number(source)||0)}${idMode()==='tag'?'t':'n'}}`}
function baseToken(tok){const m=String(tok||'').match(/^\{([^}@]+)(?:@[^}]*)?\}$/);return m?m[1]:''}
function selectedCamera(source){return cameras.get(Number(source))||registry.find(c=>Number(c.number)===Number(source))||null}
function firstConnected(){return [...cameras.values()].filter(c=>c.connected).sort((a,b)=>a.number-b.number)[0]||null}
function lowest(field){
 const live=[...cameras.values()].filter(c=>c.connected&&Number.isFinite(Number(c[field])));
 if(!live.length)return null;
 return live.reduce((a,b)=>Number(b[field])<Number(a[field])?b:a);
}
function aggregate(){
 const b=lowest('battery'),r=lowest('remain'),live=[...cameras.values()].filter(c=>c.connected);
 const h=live.find(c=>c.hot),e=live.find(c=>c.error);
 return {battery:b?Number(b.battery):null,batteryCamera:b,remain:r?Number(r.remain):null,remainCamera:r,hot:!!h,hotCamera:h||null,error:!!e,count:live.length};
}
function sourceValue(base,c,ctx={}){
 if(!c||!c.connected){
  if(base==='state')return 'OFF';
  if(base==='batt')return 'B:--';
  if(base==='rectf')return 'T:--';
  if(base==='recdur')return '--:--';
  if(base==='mode')return '---';
  if(base==='res'||base==='eis')return '---';
  if(base==='fps')return '--';
  if(base==='rcap')return '---';
  return '';
 }
 if(base==='state')return c.error?'ERR':c.recording?'REC':'RDY';
 if(base==='batt')return Number.isFinite(Number(c.battery))?'B:'+Math.max(0,Math.min(100,Number(c.battery))):'B:--';
 if(base==='rectf')return Number.isFinite(Number(c.remain))?'T:'+Math.max(0,Math.floor(Number(c.remain)))+'m':'T:--';
 if(base==='recdur')return c.recording?(c.recdur||ctx.elapsed||'00:42'):'00:00';
 if(base==='mode')return c.mode||'VIDEO';
 if(base==='res')return c.res||'4K';
 if(base==='fps')return c.fps||'60';
 if(base==='eis')return c.eis||'HS';
 if(base==='rcap')return c.rcap||'128GB';
 return '';
}
function resolvePreviewToken(spec,ctx={}){
 const p=parseToken(spec);if(!p)return null;
 if(p.source===0){
  if(p.base==='state')return ctx.aggregateState||'RDY';
  if(p.base==='batt'){const c=lowest('battery');return sourceValue('batt',c,ctx)+(c?suffix(c,p.mode):'')}
  if(p.base==='rectf'){const c=lowest('remain');return sourceValue('rectf',c,ctx)+(c?suffix(c,p.mode):'')}
  const c=firstConnected();return sourceValue(p.base,c,ctx)+(c?suffix(c,p.mode):'');
 }
 const c=selectedCamera(p.source);
 return sourceValue(p.base,c,ctx)+suffix(c||{number:p.source,label:''},p.mode);
}
function hasPinnedStatus(tpl){return /\{state(?:only)?@\d+[nt]\}/.test(String(tpl||''))}

function capsFor(c){
 const type=Number(c?.type);
 // Only disable fields when support is definitely known to be absent.
 // Caddx/Orca currently reports battery, recording/mode and storage capacity,
 // but not recording-time-remaining, normalized resolution/FPS or EIS.
 if(type===2)return {rectf:false,res:false,fps:false,eis:false};
 return {};
}
function maxBaseLen(base,source){
 if(base==='state')return source>0?3:7;
 return {batt:5,recdur:5,mode:9,res:4,fps:3,eis:3,rectf:6,rcap:5}[base]||8;
}
function tokenWorst(tok){
 const raw=String(tok||'').replace(/^\{|\}$/g,'');const p=parseToken(raw);const base=p?.base||baseToken('{'+raw+'}');
 let n=maxBaseLen(base,p?.source||0);
 if(p){
  const identifiable=p.source>0||base==='batt'||base==='rectf';
  if(identifiable)n+=1+(p.mode==='tag'?7:2);
 }
 return n;
}
function templateWorst(tpl){
 const a=String(tpl||'').match(/\{[^}]+\}/g)||[];if(!a.length)return 0;
 return a.reduce((n,t)=>n+tokenWorst(t),0)+Math.max(0,a.length-1);
}
function cameraList(){return registry.length?registry:[...cameras.values()].sort((a,b)=>a.number-b.number)}
function optionLabel(c){return `C${c.number}${cleanLabel(c.label)?' · '+cleanLabel(c.label):''}${c.connected?' · connected':''}`}

function enhanceBuilder(b){
 if(!b||b.dataset.fpsMultiOsd==='1')return;b.dataset.fpsMultiOsd='1';
 const input=b.querySelector('input[id],textarea[id]');if(!input)return;
 const tools=document.createElement('div');tools.className='fps-multiosd-builder-tools';
 tools.innerHTML='<label>Source <select class="fps-multiosd-source"><option value="0">Auto / lowest where applicable</option></select></label><span class="fps-multiosd-capacity">0 / 16</span><div class="fps-multiosd-builder-note"></div>';
 const menu=b.querySelector('.fps-menu');b.insertBefore(tools,menu||b.lastChild);
 const sel=tools.querySelector('select'),cap=tools.querySelector('.fps-multiosd-capacity'),note=tools.querySelector('.fps-multiosd-builder-note');
 function fill(){const old=sel.value;sel.innerHTML='<option value="0">Auto / lowest where applicable</option>';for(const c of cameraList())sel.add(new Option(optionLabel(c),String(c.number)));if([...sel.options].some(o=>o.value===old))sel.value=old}
 function redrawChips(){
  const toks=input.value.match(/\{[^}]+\}/g)||[],chips=[...b.querySelectorAll('.fps-chips button')];
  chips.forEach((x,i)=>{const t=toks[i];if(!t)return;const raw=t.slice(1,-1),p=parseToken(raw);if(!p)return;const src=p.source===0?'AUTO':('C'+p.source);x.textContent=`${FIELD_NAME[p.base]||p.base} · ${src} ×`});
 }
 function refreshButtons(){
  const adv=advancedOn();tools.style.display=adv?'grid':'none';fill();
  const used=templateWorst(input.value);cap.textContent=`${used} / ${LIMIT} chars max`;cap.classList.toggle('over',used>LIMIT);note.textContent=used>LIMIT?'This line is too long for Betaflight. Remove an element before applying.':'';
  const source=Number(sel.value)||0,c=source?selectedCamera(source):null,caps=capsFor(c);
  for(const btn of b.querySelectorAll('.fps-menu button')){
   const base=FIELD_TOKEN[btn.textContent.trim()];if(!base)continue;
   if(!btn.__fpsOriginalClick)btn.__fpsOriginalClick=btn.onclick;
   const tok=tokenFor(base,source),existing=input.value.match(/\{[^}]+\}/g)||[];
   const candidate=[...existing,tok].join(' '),unsupported=c&&caps[base]===false,duplicate=existing.includes(tok),overflow=templateWorst(candidate)>LIMIT;
   btn.disabled=adv&&(unsupported||duplicate||overflow);
   btn.title=!adv?'':unsupported?'This saved camera is known not to provide this telemetry.':duplicate?'That element/source is already in this OSD line.':overflow?`Would exceed Betaflight's ${LIMIT}-character text limit.`:'';
   btn.onclick=adv?(()=>{if(btn.disabled)return;input.value=candidate;input.dispatchEvent(new Event('input',{bubbles:true}));b.querySelector('.fps-menu').style.display='none';setTimeout(()=>{redrawChips();refreshButtons()},0)}):btn.__fpsOriginalClick;
  }
  redrawChips();
 }
 sel.addEventListener('change',refreshButtons);input.addEventListener('input',()=>setTimeout(refreshButtons,0));
 tools._refresh=refreshButtons;refreshButtons();
}
function refreshBuilders(){document.querySelectorAll('.fps-builder').forEach(enhanceBuilder);document.querySelectorAll('.fps-multiosd-builder-tools').forEach(t=>t._refresh?.())}

function ensureAdvancedPanel(){
 if($('fpsAdvancedMultiOsdPanel'))return;
 const top=document.querySelector('.fps-top');if(!top)return;
 const sec=document.createElement('section');sec.id='fpsAdvancedMultiOsdPanel';sec.className='fps-section fps-advanced-multiosd';
 sec.innerHTML=`<div class="fps-section-head"><div><strong>Advanced Multi Cam OSD <span class="fps-multiosd-exp">EXPERIMENTAL</span></strong><div class="fps-section-desc">Optional per-camera OSD control for Multi Cam. Leave this off for the cleanest setup.</div></div><label class="switch"><input type="checkbox" id="fpsAdvancedMultiOsd"><span class="track"><span class="thumb"></span></span></label></div><div class="fps-section-body"><div class="fps-multiosd-default"><strong>Default Multi Cam OSD</strong><br>With Advanced OFF, FreeCLinker stays exactly as it is now: battery shows the <strong>lowest battery</strong> reported by any connected camera, remaining time shows the <strong>least recording time</strong>, and temperature warnings use the hottest camera. The source can change automatically as conditions change. Normal values stay uncluttered — no C1/C2 suffixes.</div><div class="fps-multiosd-ids"><label>Camera identifier<select id="fpsMultiOsdIdentifier"><option value="number">Camera number — C1, C2…</option><option value="tag">Saved camera tag — FRONT, REAR…</option></select></label><div class="fps-note">Camera numbers are persistent saved-camera IDs, not connection order. Label the physical cameras in the Cameras tab if you want names, or a small C1/C2 tape label works perfectly. Tag mode falls back to C# when no tag is saved.</div></div><div class="fps-note">Advanced mode adds a Source selector to every OSD box for both Betaflight methods. A camera-specific Status can show OFF only when that saved camera is absent; normal Single Cam and default Multi Cam keep RDY / REC / ERR behaviour.</div></div>`;
 top.insertAdjacentElement('afterend',sec);
 const chk=$('fpsAdvancedMultiOsd'),ids=$('fpsMultiOsdIdentifier');chk.checked=localStorage.getItem(ADV_KEY)==='1';ids.value=idMode();
 chk.addEventListener('change',()=>{
  if(chk.checked){saveJson(SIMPLE_KEY,readTemplates());const saved=loadJson(ADV_TPL_KEY);if(saved)writeTemplates(saved);localStorage.setItem(ADV_KEY,'1')}
  else{saveJson(ADV_TPL_KEY,readTemplates());const simple=loadJson(SIMPLE_KEY);if(simple)writeTemplates(simple);localStorage.setItem(ADV_KEY,'0')}
  refreshAll();
 });
 ids.addEventListener('change',()=>{localStorage.setItem(ID_KEY,ids.value==='tag'?'tag':'number');rewriteIdentifierMode();refreshAll()});
}
function rewriteIdentifierMode(){
 if(!advancedOn())return;const ch=idMode()==='tag'?'t':'n';
 for(const id of TEMPLATE_IDS){const e=$(id);if(!e)continue;const next=e.value.replace(/@([0-9]+)[nt](?=\})/g,'@$1'+ch);if(next!==e.value){e.value=next;e.dispatchEvent(new Event('input',{bubbles:true}))}}
}
function refreshPanel(){
 const p=$('fpsAdvancedMultiOsdPanel');if(!p)return;const visible=multiOn();p.style.display=visible?'':'none';
 const body=p.querySelector('.fps-section-body');if(body)body.style.display=advancedOn()?'grid':'none';
 const chk=$('fpsAdvancedMultiOsd');if(chk)chk.checked=localStorage.getItem(ADV_KEY)==='1';
}

function mergeRegistry(list){
 registry=Array.isArray(list)?list.map((c,i)=>({number:Number(c.number)||i+1,label:cleanLabel(c.label),type:c.type,connected:!!c.connected,name:c.name||'',battery:c.battery,remain:c.remain,recording:c.recording,hot:c.hot})):[];
 for(const c of registry){const prev=cameras.get(c.number)||{};cameras.set(c.number,{number:c.number,label:c.label,type:c.type,connected:c.connected,battery:c.battery??prev.battery??(80-c.number*6),remain:c.remain!=null&&c.remain>=0?Math.round(c.remain/60):prev.remain??(50-c.number*7),recording:c.recording==null?!!prev.recording:c.recording===1,hot:c.hot==null?!!prev.hot:c.hot===1,error:!!prev.error,mode:prev.mode||'VIDEO',res:prev.res||'4K',fps:prev.fps||'60',eis:prev.eis||'HS',rcap:prev.rcap||'128GB'})}
 refreshAll();
}
function ensureDemoDefaults(){if(cameras.size)return;[['FRONT',82,41],['REAR',69,14],['ACT 5',76,32],['360CAM',61,55]].forEach((x,i)=>cameras.set(i+1,{number:i+1,label:x[0],type:i<2?1:(i===2?0:5),connected:i<2,battery:x[1],remain:x[2],recording:false,hot:false,error:false,mode:'VIDEO',res:'4K',fps:'60',eis:'HS',rcap:'128GB'}))}
function ensurePreviewSimulator(){
 const sim=document.querySelector('#fpsIntegratedPreview .fps-preview-sim');if(!sim)return;
 let box=$('fpsMultiOsdSimulator');if(!box){box=document.createElement('div');box.id='fpsMultiOsdSimulator';box.className='fps-multiosd-simulator';sim.appendChild(box)}
 const show=advancedOn()&&multiOn();box.style.display=show?'grid':'none';if(!show)return;
 ensureDemoDefaults();
 const live=cameraList();box.innerHTML=`<div class="fps-multiosd-sim-head"><strong>Advanced Multi Cam preview</strong><span>${demo()?'Demo telemetry':'Live/preview telemetry'}</span></div><div class="fps-note">Change each camera independently to test lowest-value switching, pinned sources, OFF state and warnings.</div>`;
 for(const meta of live){const c=cameras.get(Number(meta.number))||meta;const card=document.createElement('div');card.className='fps-multiosd-sim-card';card.innerHTML=`<strong>C${c.number}${cleanLabel(c.label)?' · '+cleanLabel(c.label):''}</strong><label>Connected <input data-k="connected" type="checkbox" ${c.connected?'checked':''}></label><label>Battery <input data-k="battery" type="number" min="0" max="100" value="${Number.isFinite(Number(c.battery))?Number(c.battery):69}"></label><label>Time left <input data-k="remain" type="number" min="0" max="999" value="${Number.isFinite(Number(c.remain))?Number(c.remain):45}"></label><label>State <select data-k="state"><option value="RDY" ${!c.recording&&!c.error?'selected':''}>RDY</option><option value="REC" ${c.recording?'selected':''}>REC</option><option value="ERR" ${c.error?'selected':''}>ERR</option></select></label><label>Hot <input data-k="hot" type="checkbox" ${c.hot?'checked':''}></label>`;
  card.addEventListener('input',e=>{const k=e.target.dataset.k;if(!k)return;const x=cameras.get(c.number)||c;if(k==='connected')x.connected=e.target.checked;else if(k==='battery')x.battery=Math.max(0,Math.min(100,Number(e.target.value)||0));else if(k==='remain')x.remain=Math.max(0,Number(e.target.value)||0);else if(k==='hot')x.hot=e.target.checked;else if(k==='state'){x.recording=e.target.value==='REC';x.error=e.target.value==='ERR'}cameras.set(c.number,x);refreshBuilders()});
  box.appendChild(card);
 }
}
function refreshAll(){refreshPanel();refreshBuilders();ensurePreviewSimulator()}

function init(){
 ensureAdvancedPanel();ensureDemoDefaults();refreshAll();
 document.addEventListener('fps-camera-registry-update',e=>mergeRegistry(e.detail));
 document.addEventListener('fps-demo-camera-connections',e=>{if(Array.isArray(e.detail?.cameras))mergeRegistry(e.detail.cameras)});
 document.addEventListener('change',e=>{if(e.target?.id==='fpsMultiCamSync'||e.target?.id==='fpsExperimentalMaster')setTimeout(refreshAll,0)},true);
 const obs=new MutationObserver(()=>{if(document.querySelector('.fps-builder'))refreshBuilders();if($('fpsIntegratedPreview'))ensurePreviewSimulator()});obs.observe(document.body,{childList:true,subtree:true});
 window.fpsMultiCamOsd={advancedOn,multiOn,resolvePreviewToken,aggregate,hasPinnedStatus,cameras,refresh:refreshAll};
}
const css=document.createElement('style');css.textContent=`
.fps-advanced-multiosd{display:none}.fps-multiosd-exp{font-size:9px;color:#b45309}.fps-multiosd-default{padding:11px 13px;border:1px solid rgba(124,58,237,.38);border-radius:8px;background:rgba(124,58,237,.08);font-size:12px;line-height:1.5}.fps-multiosd-ids{display:grid;gap:7px}.fps-multiosd-ids label{display:grid;gap:6px}.fps-multiosd-builder-tools{display:none;grid-template-columns:minmax(0,1fr) auto;gap:7px 10px;align-items:end;margin:8px 0;padding:9px;border:1px solid rgba(124,58,237,.3);border-radius:8px}.fps-multiosd-builder-tools label{display:grid;gap:4px;font-size:10px;color:var(--muted,#aaa)}.fps-multiosd-capacity{font:700 10px monospace;color:#a78bfa;white-space:nowrap}.fps-multiosd-capacity.over{color:#ff4d5e}.fps-multiosd-builder-note{grid-column:1/-1;min-height:0;color:#ff7b86;font-size:10px}.fps-multiosd-simulator{grid-column:1/-1;gap:8px;margin-top:3px;padding-top:10px;border-top:1px solid rgba(124,58,237,.3)}.fps-multiosd-sim-head{display:flex;justify-content:space-between;gap:8px;font-size:11px;color:#c4b5fd}.fps-multiosd-sim-card{display:grid;grid-template-columns:1fr 1fr;gap:6px 10px;padding:9px;border:1px solid rgba(124,58,237,.3);border-radius:8px}.fps-multiosd-sim-card>strong{grid-column:1/-1}.fps-multiosd-sim-card label{font-size:10px!important}.fps-multiosd-sim-card input[type=number]{width:58px!important}.fps-multiosd-sim-card select{min-width:70px!important}@media(max-width:640px){.fps-multiosd-builder-tools,.fps-multiosd-sim-card{grid-template-columns:1fr}.fps-multiosd-sim-card>strong{grid-column:auto}}
`;document.head.appendChild(css);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,360));else setTimeout(init,360);
})();