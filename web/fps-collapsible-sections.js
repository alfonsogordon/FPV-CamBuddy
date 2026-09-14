(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const STORE='fpsCollapsedMenusV2';
function load(){try{return JSON.parse(localStorage.getItem(STORE)||'{}')}catch{return{}}}
function save(v){localStorage.setItem(STORE,JSON.stringify(v))}
function setEnabled(root,toggle){root.classList.toggle('fps-menu-enabled',!!toggle?.checked)}
function addButton(header,key,getTargets,toggle){
 if(!header||header.querySelector(':scope > .fps-menu-collapse-btn'))return;
 const btn=document.createElement('button');btn.type='button';btn.className='fps-menu-collapse-btn';
 let collapsed=!!load()[key];
 function draw(){
  for(const el of getTargets())el.classList.toggle('fps-menu-collapsed-target',collapsed);
  btn.textContent=collapsed?'Show':'Hide';
  btn.setAttribute('aria-expanded',collapsed?'false':'true');
  btn.title=collapsed?'Show settings':'Hide settings';
  setEnabled(header.closest('.fps-menu-collapse-root')||header.parentElement,toggle);
 }
 btn.addEventListener('click',e=>{e.preventDefault();e.stopPropagation();collapsed=!collapsed;const s=load();s[key]=collapsed;save(s);draw()});
 const sw=header.querySelector(':scope > .switch');
 if(sw)header.insertBefore(btn,sw);else header.appendChild(btn);
 toggle?.addEventListener('change',()=>setEnabled(header.closest('.fps-menu-collapse-root')||header.parentElement,toggle));
 draw();
}
function enhanceOsd(){
 const master=document.getElementById('fpsOsdMaster');const top=master?.closest('.fps-top');const card=top?.closest('.config-card');if(!master||!top||!card||card.dataset.fpsMenuCollapse==='1')return;
 card.dataset.fpsMenuCollapse='1';card.classList.add('fps-menu-collapse-root');
 const header=top.querySelector('.fps-master');if(!header)return;
 addButton(header,'osd-templates',()=>[...card.children].filter(x=>x!==card.querySelector('.card-header')&&x!==top),master);
}
function enhanceSection(toggleId,key){
 const toggle=document.getElementById(toggleId);const sec=toggle?.closest('.fps-section');const head=sec?.querySelector(':scope > .fps-section-head');const body=sec?.querySelector(':scope > .fps-section-body');
 if(!toggle||!sec||!head||!body||sec.dataset.fpsMenuCollapse==='1')return;
 sec.dataset.fpsMenuCollapse='1';sec.classList.add('fps-menu-collapse-root');
 addButton(head,key,()=>[body],toggle);
}
function scan(){
 enhanceOsd();
 enhanceSection('fpsAuxMaster','aux-camera-controls');
 enhanceSection('fpsAdvancedMultiOsd','advanced-multicam-osd');
}
const style=document.createElement('style');style.id='fpsCollapsibleSectionsStyle';style.textContent=`
.fps-menu-collapse-root{transition:border-color .18s ease,box-shadow .18s ease}.fps-menu-collapse-root.fps-menu-enabled{border-color:rgba(124,58,237,.72)!important;box-shadow:0 0 0 1px rgba(124,58,237,.17),0 0 16px rgba(124,58,237,.07)}
.fps-menu-collapsed-target{display:none!important}.fps-menu-collapse-btn{margin-left:auto;margin-right:9px;border:1px solid rgba(167,139,250,.34)!important;border-radius:7px!important;background:rgba(124,58,237,.07)!important;color:#ddd6fe!important;padding:5px 10px!important;min-width:52px!important;font-size:10px!important;font-weight:800!important;letter-spacing:.02em;cursor:pointer}.fps-menu-collapse-btn:hover{background:rgba(124,58,237,.15)!important;border-color:rgba(167,139,250,.58)!important;color:#fff!important}.fps-menu-enabled .fps-menu-collapse-btn{border-color:rgba(167,139,250,.52)!important;color:#f0e8ff!important}.fps-master,.fps-section-head{gap:8px}
`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(scan,600));else setTimeout(scan,600);
new MutationObserver(()=>scan()).observe(document.documentElement,{childList:true,subtree:true});
})();