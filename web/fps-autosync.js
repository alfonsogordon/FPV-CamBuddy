(()=>{
'use strict';
const scripts=['fps-autosync-core.js','fps-multicam-osd-element-sources.js'];
function load(i){if(i>=scripts.length)return;const s=document.createElement('script');s.src=scripts[i]+'?v=20260915-7';s.onload=()=>load(i+1);s.onerror=()=>console.error('[FPS] Failed to load',scripts[i]);document.body.appendChild(s)}
load(0);
})();