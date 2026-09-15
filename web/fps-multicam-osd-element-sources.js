(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const TOKEN_RE=/\{[^}]+\}/g;
const FIELD_NAME={state:'Status',stateonly:'Status',batt:'Battery',recdur:'Recording Duration',mode:'Mode',res:'Resolution',fps:'FPS',eis:'Stabilisation',rectf:'Time Remaining',rcap:'Storage Remaining'};
const PREVIEW_DEFAULTS={1:{battery:82,remain:41},2:{battery:69,remain:74},3:{battery:76,remain:32},4:{battery:61,remain:55}};
const advanced=()=>!!window.fpsMultiCamOsd?.advancedOn?.();
function parse(tok){const raw=String(tok||'').replace(/^\{|\}$/g,'');const m=raw.match(/^([a-z0-9]+)(?:@(\d+)([nt]))?$/i);return m?{base:m[1],source:Number(m[2])||0,mode:m[3]||null}:null}
function cameraList(){const api=window.fpsMultiCamOsd,list=api?.cameraList?.();if(Array.isArray(list)&&list.length)return list;const map=api?.cameras;if(map?.values)return [...map.values()].sort((a,b)=>(a.number||0)-(b.number||0));return[]}
function optionText(c){const tag=String(c?.label||'').trim();return `C${c.number}${tag?' · '+tag:''}${c.connected?' · connected':''}`}
function token(base,source){if(!source)return `{${base}}`;const mode=localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n';return `{${base}@${source}${mode}}`}
function changeSource(input,index,source){const toks=input.value.match(TOKEN_RE)||[];if(!toks[index])return;const p=parse(toks[index]);if(!p)return;const next=token(p.base,source);if(toks.some((t,j)=>j!==index&&t===next))return false;toks[index]=next;input.value=toks.join(' ');input.dispatchEvent(new Event('input',{bubbles:true}));input.dispatchEvent(new Event('change',{bubbles:true}));return true}
function redecorateAfterLegacyRefresh(builder){
 [0,16,50,150,350].forEach(ms=>setTimeout(()=>decorate(builder),ms));
}
function buildSelect(chip,input,index,p){
 const sel=document.createElement('select');sel.className='fps-element-source';sel.title='Source camera for this OSD element';sel.add(new Option('AUTO','0'));for(const c of cameraList())sel.add(new Option(optionText(c),String(c.number)));sel.value=String(p.source||0);
 for(const ev of ['pointerdown','mousedown','click'])sel.addEventListener(ev,e=>e.stopPropagation());
 sel.addEventListener('change',e=>{e.stopPropagation();const previous=p.source||0,next=Number(sel.value)||0,builder=chip.closest('.fps-builder');if(!changeSource(input,index,next)){sel.value=String(previous);sel.title='That element/source is already in this OSD line.';return}sel.title='Source camera for this OSD element';redecorateAfterLegacyRefresh(builder)});
 return sel
}
function decorate(builder){
 if(!builder)return;const tools=builder.querySelector('.fps-multiosd-builder-tools');if(tools)tools.style.display='none';
 const input=builder.querySelector('input[id],textarea[id]');const chipBox=builder.querySelector('.fps-chips');if(!input||!chipBox)return;
 const toks=input.value.match(TOKEN_RE)||[],chips=[...chipBox.querySelectorAll('button')];
 chips.forEach((chip,i)=>{const p=parse(toks[i]);if(!p)return;if(!advanced()){if(chip.dataset.fpsElementSource==='1'){chip.dataset.fpsElementSource='0';chip.dataset.fpsSourceSig='';chip.textContent=(FIELD_NAME[p.base]||p.base)+' ×'}return}
 const sig=`${p.base}|${p.source}|${cameraList().map(c=>`${c.number}:${c.label||''}:${c.connected?1:0}`).join(',')}`;
 const current=chip.querySelector('.fps-element-source');if(chip.dataset.fpsElementSource==='1'&&current){if(current.value!==String(p.source||0))current.value=String(p.source||0);if(chip.dataset.fpsSourceSig===sig)return}
 chip.dataset.fpsElementSource='1';chip.dataset.fpsSourceSig=sig;chip.textContent='';
 const name=document.createElement('span');name.className='fps-element-name';name.textContent=(FIELD_NAME[p.base]||p.base)+' · ';
 const close=document.createElement('span');close.className='fps-element-remove';close.textContent=' ×';
 chip.append(name,buildSelect(chip,input,i,p),close)
 })
}
function syncStatusOptions(){
 const sec=document.querySelector('.fps-status-shared');if(!sec)return;
 if(!advanced())return;
 const legacy=document.getElementById('fpsBfMode')?.value==='legacy';
 const ids=legacy?['pilotTpl','craftTpl']:['osd1','osd2','osd3','osd4'];
 const enabled=legacy?[document.getElementById('fpsPilotMaster')?.checked,document.getElementById('fpsCraftMaster')?.checked]:[1,2,3,4].map(n=>document.getElementById('fpsMsg'+n)?.checked);
 const used=ids.some((id,i)=>enabled[i]&&/\{state(?:only)?(?:@\d+[nt])?\}/i.test(String(document.getElementById(id)?.value||'')));
 sec.style.display=document.getElementById('fpsOsdMaster')?.checked&&used?'':'none';
}
function seedPreview(){
 if(!advanced())return;const api=window.fpsMultiCamOsd,map=api?.cameras;if(!map?.get)return;let changed=false;
 document.querySelectorAll('#fpsMultiOsdSimulator .fps-multiosd-cam').forEach(card=>{const n=Number(card.dataset.cam),d=PREVIEW_DEFAULTS[n];if(!d)return;const c=map.get(n);if(!c)return;const connected=card.querySelector('input[data-k="connected"]'),rec=card.querySelector('input[data-k="recording"]'),batt=card.querySelector('input[data-k="battery"]'),remain=card.querySelector('input[data-k="remain"]');
  if(card.dataset.fpsPreviewSeeded!=='1'){
   if(!Number.isFinite(Number(c.battery))||Number(c.battery)<=0){c.battery=d.battery;if(batt)batt.value=String(d.battery);changed=true}else if(batt&&Number(batt.value)<=0)batt.value=String(c.battery);
   if(!Number.isFinite(Number(c.remain))||Number(c.remain)<=0){c.remain=d.remain;if(remain)remain.value=String(d.remain);changed=true}else if(remain&&Number(remain.value)<=0)remain.value=String(c.remain);
   card.dataset.fpsPreviewSeeded='1';
  }
  const isConnected=!!connected?.checked;card.classList.toggle('fps-preview-cam-off',!isConnected);if(rec)rec.disabled=!isConnected;if(batt)batt.disabled=!isConnected;if(remain){remain.disabled=!isConnected;remain.max='999';remain.title='Time remaining supports up to 999 minutes (3 digits).'}
 });
 if(changed)requestAnimationFrame(()=>api?.refresh?.());
}
function ensureStyle(){if(document.getElementById('fpsElementSourceUiStyle'))return;const s=document.createElement('style');s.id='fpsElementSourceUiStyle';s.textContent='#fpsMultiOsdSimulator .fps-preview-cam-off .fps-multiosd-toggle:not(:first-child),#fpsMultiOsdSimulator .fps-preview-cam-off .fps-multiosd-value-row{opacity:.38}#fpsMultiOsdSimulator .fps-preview-cam-off input:disabled{cursor:not-allowed}';document.head.appendChild(s)}
function refresh(){ensureStyle();document.querySelectorAll('.fps-builder').forEach(decorate);syncStatusOptions();seedPreview()}
let queued=false;const queue=()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refresh()})};
new MutationObserver(queue).observe(document.documentElement,{childList:true,subtree:true,characterData:true});
document.addEventListener('input',queue,true);document.addEventListener('change',queue,true);window.addEventListener('fps-camera-registry-update',queue);window.addEventListener('fps-config-read-complete',queue);setInterval(refresh,250);queue();
})();
