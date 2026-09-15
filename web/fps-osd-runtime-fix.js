(()=>{
'use strict';
function templateForLine(line){
 const rows=[...document.querySelectorAll('#fpsPreviewRows .fps-preview-line')];
 const i=rows.indexOf(line); if(i<0)return'';
 const legacy=document.getElementById('fpsBfMode')?.value==='legacy';
 if(legacy){const ids=['pilotTpl','craftTpl'];return document.getElementById(ids[i])?.value||''}
 const enabled=[1,2,3,4].filter(n=>document.getElementById('fpsMsg'+n)?.checked);
 return document.getElementById('osd'+enabled[i])?.value||'';
}
function parseSources(tpl){return (String(tpl).match(/\{[^}]+@\d+[nt]\}/g)||[]).map(t=>{const m=t.match(/@(\d+)([nt])\}/);return m?{n:Number(m[1]),mode:m[2]}:null}).filter(Boolean)}
function clean(v){return String(v||'').trim().replace(/\s+/g,' ')}
function idFor(n,mode){if(mode==='t'){const c=window.fpsMultiCamOsd?.cameras?.get?.(n),tag=clean(c?.label);return tag||('C'+n)}return'C'+n}
function dedupeLine(line){
 const tpl=templateForLine(line),specs=parseSources(tpl); if(specs.length<2)return;
 const uniq=[...new Set(specs.map(s=>s.n))]; if(uniq.length!==1)return;
 const id=idFor(uniq[0],specs[0].mode),esc=id.replace(/[.*+?^${}()|[\]\\]/g,'\\$&');
 let seen=false; const next=String(line.textContent||'').replace(new RegExp('-'+esc+'(?=\\s|$)','g'),m=>{if(seen)return'';seen=true;return m}).replace(/\s+/g,' ').trim();
 if(next!==line.textContent)line.textContent=next;
}
function fix(){document.querySelectorAll('#fpsPreviewRows .fps-preview-line').forEach(dedupeLine)}
let busy=false;new MutationObserver(()=>{if(busy)return;busy=true;requestAnimationFrame(()=>{busy=false;fix()})}).observe(document.documentElement,{childList:true,subtree:true,characterData:true});
setInterval(fix,120);
})();