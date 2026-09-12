(()=>{
'use strict';
const q=id=>document.getElementById(id);
function init(){
  const master=q('fpsOsdEnable'), bf=q('bf45Compat'), osdCard=q('osd1')?.closest('.config-card'), methodCard=bf?.closest('.config-card');
  if(!master||!bf||!osdCard||!methodCard||q('fpsOsdMethod'))return;
  const masterHead=master.closest('.fps-feature-master');
  if(!masterHead)return;

  methodCard.classList.add('fps-osd-method-card');
  masterHead.after(methodCard);
  methodCard.querySelector('.card-header')?.classList.add('fps-hidden');
  bf.closest('.cfg-field')?.classList.add('fps-hidden');

  const method=document.createElement('div');
  method.className='cfg-field fps-osd-method-row';
  method.innerHTML='<div class="cfg-field-text"><div class="cfg-field-name">Betaflight Version / OSD Method</div><div class="cfg-field-desc">The controls below stay the same; only the Betaflight text destination changes.</div></div><select id="fpsOsdMethod"><option value="current">Betaflight 2026.6+ — Custom Messages 1–4</option><option value="legacy">Betaflight 4.4 – 2025.12 — Pilot + Craft Name</option></select>';
  methodCard.insertBefore(method,methodCard.firstChild);
  const sel=q('fpsOsdMethod');
  sel.value=bf.checked?'legacy':'current';

  // The old compatibility card contains both legacy-only destinations and the
  // shared Custom Message / Camera Warnings controls. Mark them separately so
  // switching Betaflight versions never hides common functionality.
  const pilot=q('pilotTpl')?.closest('.fps-builder');
  const craft=q('craftTpl')?.closest('.fps-builder');
  pilot?.classList.add('fps-legacy-only');
  craft?.classList.add('fps-legacy-only');
  const shared=q('fpvPreArmText')?.closest('.cfg-field') || q('fpvLowBatt')?.closest('.cfg-field');
  shared?.classList.add('fps-shared-osd-options');
  methodCard.querySelector('.card-footer')?.classList.add('fps-shared-osd-footer');

  function applyMethod(){
    const legacy=sel.value==='legacy';
    if(bf.checked!==legacy){bf.checked=legacy;bf.dispatchEvent(new Event('change',{bubbles:true}));}

    // Current BF uses the four Custom Message builders; legacy BF uses Pilot/Craft.
    osdCard.classList.toggle('fps-mode-inactive',legacy);
    methodCard.classList.remove('fps-mode-inactive');
    methodCard.classList.toggle('fps-legacy-mode',legacy);
    methodCard.classList.toggle('fps-current-mode',!legacy);

    // fps-ui-v2 may have collapsed all rows when legacy mode is off. Shared
    // controls are explicitly restored here after its change handler runs.
    requestAnimationFrame(()=>{
      shared?.classList.remove('fps-collapsed');
      methodCard.querySelector('.fps-shared-osd-footer')?.classList.remove('fps-collapsed');
      pilot?.classList.toggle('fps-hidden',!legacy);
      craft?.classList.toggle('fps-hidden',!legacy);
    });
  }
  function applyMaster(){
    const on=master.checked;
    methodCard.classList.toggle('fps-osd-master-off',!on);
    method.classList.toggle('fps-collapsed',!on);
  }
  sel.addEventListener('change',applyMethod);
  master.addEventListener('change',applyMaster);
  bf.addEventListener('change',()=>{sel.value=bf.checked?'legacy':'current';applyMethod();applyMaster()});
  applyMethod();
  applyMaster();
}
const css=document.createElement('style');
css.textContent=`
.fps-mode-inactive>.fps-feature-master{display:flex!important}
.fps-osd-method-card{margin:0!important;border:0!important;border-radius:0!important;background:transparent!important;box-shadow:none!important;opacity:1!important}
.fps-osd-method-card>.fps-osd-method-row{display:flex!important;padding:14px 18px!important;border-bottom:1px solid rgba(124,58,237,.22)}
.fps-osd-method-card.fps-osd-master-off{display:none!important}
.fps-osd-method-card>.fps-shared-osd-options{display:block!important;opacity:1!important}
.fps-osd-method-card>.fps-shared-osd-footer{display:flex!important}
.fps-osd-method-card.fps-current-mode>.fps-legacy-only{display:none!important}
.fps-osd-method-card.fps-legacy-mode>.fps-legacy-only{display:block!important}
.fps-osd-method-card.fps-current-mode .fps-unavailable{display:none!important}
#fpsOsdMethod{max-width:330px}
`;
document.head.appendChild(css);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,80));else setTimeout(init,80);
})();