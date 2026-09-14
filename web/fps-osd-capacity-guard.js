(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const LIMIT=16;
const BASE={
 Status:'state',Battery:'batt','Recording Duration':'recdur',Mode:'mode',Resolution:'res',FPS:'fps',Stabilisation:'eis','Time Remaining':'rectf','Storage Remaining':'rcap'
};
const MAX={state:7,batt:5,recdur:5,mode:9,res:4,fps:3,eis:3,rectf:6,rcap:5};
const TOKEN_RE=/\{[^}]+\}/g;
function advanced(){return !!window.fpsMultiCamOsd?.advancedOn?.()}
function idMode(){return localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n'}
function parse(raw){
 const s=String(raw||'').replace(/^\{|\}$/g,'');
 const m=s.match(/^([a-z0-9]+)@(\d+)([nt])$/i);
 return m?{base:m[1],source:Number(m[2]),mode:m[3]}:{base:s.replace(/@.*$/,''),source:null,mode:null};
}
function worstToken(tok){
 const p=parse(tok);let n=MAX[p.base]||8;
 if(p.source!==null){
  const identifiable=p.source>0||p.base==='batt'||p.base==='rectf';
  if(identifiable)n+=1+(p.mode==='t'?7:2);
 }
 return n;
}
function worst(tpl){
 const a=String(tpl||'').match(TOKEN_RE)||[];
 return a.reduce((n,t)=>n+worstToken(t),0)+Math.max(0,a.length-1);
}
function inputFor(builder){return builder?.querySelector('input[id],textarea[id]')||null}
function tokenFor(builder,base){
 if(!advanced())return `{${base}}`;
 const source=Number(builder.querySelector('.fps-multiosd-source')?.value)||0;
 return `{${base}@${source}${idMode()}}`;
}
function candidate(builder,btn){
 const input=inputFor(builder),base=BASE[btn?.textContent?.trim()];
 if(!input||!base)return null;
 const add=tokenFor(builder,base),existing=input.value.match(TOKEN_RE)||[];
 if(existing.includes(add))return input.value;
 return [...existing,add].join(' ');
}
function ensureBadge(builder){
 let badge=builder.querySelector(':scope > .fps-osd-capacity-guard');
 if(!badge){badge=document.createElement('span');badge.className='fps-osd-capacity-guard';const add=builder.querySelector('.fps-add');add?.insertAdjacentElement('afterend',badge)}
 return badge;
}
function refresh(builder){
 const input=inputFor(builder);if(!input)return;
 const used=worst(input.value),badge=ensureBadge(builder);if(badge){badge.textContent=`${used} / ${LIMIT}`;badge.classList.toggle('over',used>LIMIT);badge.title=`Worst-case Betaflight text length: ${used} of ${LIMIT} characters.`}
 let any=false;
 for(const btn of builder.querySelectorAll('.fps-menu button')){
  const c=candidate(builder,btn);if(c===null)continue;
  const overflow=worst(c)>LIMIT;
  btn.dataset.fpsCapacityBlocked=overflow?'1':'0';
  if(overflow){btn.disabled=true;btn.title=`Won't fit: this OSD box is limited to ${LIMIT} displayed characters.`}
  else if(btn.dataset.fpsCapacityWasDisabled==='1'){btn.disabled=false;btn.removeAttribute('title')}
  btn.dataset.fpsCapacityWasDisabled=overflow?'1':'0';
  if(!overflow)any=true;
 }
 const add=builder.querySelector('.fps-add');if(add){add.disabled=!any;add.title=any?'':`This OSD box is full (${LIMIT} displayed characters max). Remove an element to add another.`}
}
function refreshAll(){document.querySelectorAll('.fps-builder').forEach(refresh)}
document.addEventListener('click',e=>{
 const btn=e.target.closest?.('.fps-menu button');if(!btn)return;
 const builder=btn.closest('.fps-builder'),c=candidate(builder,btn);if(c!==null&&worst(c)>LIMIT){e.preventDefault();e.stopImmediatePropagation();refresh(builder)}
},true);
document.addEventListener('input',e=>{const b=e.target.closest?.('.fps-builder');if(b)setTimeout(()=>refresh(b),0)},true);
document.addEventListener('change',e=>{if(e.target.matches?.('.fps-multiosd-source,#fpsMultiOsdIdentifier,#fpsAdvancedMultiOsd,#fpsBfMode'))setTimeout(refreshAll,0)},true);
let queued=false;new MutationObserver(()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refreshAll()})}).observe(document.documentElement,{childList:true,subtree:true});
const style=document.createElement('style');style.textContent=`.fps-osd-capacity-guard{display:inline-block;margin-left:8px;font:700 10px/1 monospace;color:#a78bfa;vertical-align:middle}.fps-osd-capacity-guard.over{color:#ff5c6c}.fps-builder .fps-add:disabled{opacity:.42}`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(refreshAll,650));else setTimeout(refreshAll,650);
})();