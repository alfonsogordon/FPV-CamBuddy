(()=>{
'use strict';
const scripts=['fps-autosync-core.js','fps-multicam-osd.js','fps-osd-capacity-guard.js','fps-osd-runtime-fix.js'];
function load(i){if(i>=scripts.length)return;const s=document.createElement('script');s.src=scripts[i]+'?v=20260915-2';s.onload=()=>load(i+1);s.onerror=()=>console.error('[FPS] Failed to load',scripts[i]);document.body.appendChild(s)}
load(0);
})();