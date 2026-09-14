(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const STORE='fpsCollapsedMenusV4';
function load(){try{return JSON.parse(localStorage.getItem(STORE)||'{}')}catch{return{}}}
function save(v){localStorage.setItem(STORE,JSON.stringify(v))}
function enabled(root,toggle){root.classList.toggle('fps-menu-enabled',!!toggle?.checked)}
function addChevron(host,key,getTargets,toggle,root,defaultCollapsed=false){
 if(!host||host.querySelector(':scope > .fps-menu-collapse-arrow'))return;
 const btn=document.createElement('button');
 btn.type='button';btn.className='fps-menu-collapse-arrow';btn.setAttribute('aria-label','Collapse section');
 const saved=load();let collapsed=Object.prototype.hasOwnProperty.call(saved,key)?!!saved[key]:(toggle?!toggle.checked:defaultCollapsed);
 function persist(){const s=load();s[key]=collapsed;save(s)}
 function draw(){
  for(const el of getTargets())if(el)el.classList.toggle('fps-menu-collapsed-target',collapsed);
  root.classList.toggle('fps-menu-collapsed',collapsed);
  btn.classList.toggle('is-collapsed',collapsed);
  btn.setAttribute('aria-expanded',collapsed?'false':'true');
  btn.title=collapsed?'Expand section':'Collapse section';
  enabled(root,toggle);
 }
 btn.addEventListener('click',e=>{e.preventDefault();e.stopPropagation();collapsed=!collapsed;persist();draw()});
 host.appendChild(btn);
 toggle?.addEventListener('change',()=>{collapsed=!toggle.checked;persist();draw()});
 draw();
}
function enhanceOsd(){
 const master=document.getElementById('fpsOsdMaster');const top=master?.closest('.fps-top');const card=top?.closest('.config-card');const cardHead=card?.querySelector(':scope > .card-header');
 if(!master||!top||!card||!cardHead||card.dataset.fpsMenuCollapse==='1')return;
 card.dataset.fpsMenuCollapse='1';card.classList.add('fps-menu-collapse-root');
 addChevron(cardHead,'osd-templates',()=>[...card.children].filter(x=>x!==cardHead&&x!==top).concat(document.getElementById('fpsIntegratedPreview')).filter(Boolean),master,card);
}
function sectionKey(sec,toggle){
 if(toggle?.id)return 'section-'+toggle.id;
 const title=sec.querySelector(':scope > .fps-section-head strong')?.textContent||'settings';
 return 'section-'+title.toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');
}
function enhanceSection(sec){
 const head=sec?.querySelector(':scope > .fps-section-head');const body=sec?.querySelector(':scope > .fps-section-body');const toggle=head?.querySelector('input[type="checkbox"]');
 if(!sec||!head||!body||sec.dataset.fpsMenuCollapse==='1')return;
 sec.dataset.fpsMenuCollapse='1';sec.classList.add('fps-menu-collapse-root');
 addChevron(head,sectionKey(sec,toggle),()=>[body],toggle,sec,false);
}
function scan(){
 enhanceOsd();
 document.querySelectorAll('.fps-section').forEach(enhanceSection);
}
const style=document.createElement('style');style.id='fpsCollapsibleSectionsStyle';style.textContent=`
.fps-menu-collapse-root{transition:border-color .18s ease,box-shadow .18s ease,background .18s ease}.fps-menu-collapse-root.fps-menu-enabled{border-color:rgba(124,58,237,.78)!important;box-shadow:0 0 0 1px rgba(124,58,237,.2),0 0 16px rgba(124,58,237,.08)}.fps-menu-collapse-root.fps-menu-enabled.fps-menu-collapsed{background:rgba(124,58,237,.045)!important}.fps-menu-collapsed-target{display:none!important}
.fps-menu-collapse-arrow{margin-left:auto!important;width:30px!important;height:30px!important;min-width:30px!important;padding:0!important;border:0!important;background:transparent!important;position:relative!important;border-radius:7px!important;box-shadow:none!important}.fps-menu-collapse-arrow:hover{background:rgba(124,58,237,.12)!important}.fps-menu-collapse-arrow::before{content:'';position:absolute;left:10px;top:8px;width:8px;height:8px;border-right:2px solid #b8a7e8;border-bottom:2px solid #b8a7e8;transform:rotate(45deg);transition:transform .16s ease,top .16s ease,border-color .16s ease}.fps-menu-collapse-arrow.is-collapsed::before{transform:rotate(-45deg);top:10px}.fps-menu-enabled .fps-menu-collapse-arrow::before{border-color:#c4b5fd}.card-header,.fps-section-head{gap:8px}
`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(scan,600));else setTimeout(scan,600);
new MutationObserver(()=>scan()).observe(document.documentElement,{childList:true,subtree:true});
})();