(()=>{
'use strict';
const q=id=>document.getElementById(id);
function init(){
  const master=q('fpsOsdEnable'), bf=q('bf45Compat'), osd=q('osd1')?.closest('.config-card'), bfCard=bf?.closest('.config-card');
  if(!master||!bf||!osd||!bfCard||q('fpsOsdMethod'))return;
  const masterHead=master.closest('.fps-feature-master');
  if(!masterHead)return;

  // Put the Betaflight OSD method inside the OSD Templates feature.
  bfCard.classList.add('fps-osd-method-card');
  masterHead.after(bfCard);
  const oldHeader=bfCard.querySelector('.card-header');
  if(oldHeader)oldHeader.classList.add('fps-hidden');
  const oldRow=bf.closest('.cfg-field');
  if(oldRow)oldRow.classList.add('fps-hidden');

  const method=document.createElement('div');
  method.className='cfg-field fps-osd-method-row';
  method.innerHTML='<div class="cfg-field-text"><div class="cfg-field-name">Betaflight Version / OSD Method</div><div class="cfg-field-desc">Choose the OSD text method supported by your Betaflight version.</div></div><select id="fpsOsdMethod"><option value="current">Betaflight 2026.6+ — Custom Messages 1–4</option><option value="legacy">Betaflight 4.4 – 2025.12 — Pilot + Craft Name</option></select>';
  bfCard.insertBefore(method,bfCard.firstChild);
  const sel=q('fpsOsdMethod');
  sel.value=bf.checked?'legacy':'current';

  function applyMethod(){
    const legacy=sel.value==='legacy';
    if(bf.checked!==legacy){bf.checked=legacy;bf.dispatchEvent(new Event('change',{bubbles:true}));}
    // Existing FPSteVe UI handles which builders are active. Keep the OSD master visible in both modes.
    osd.classList.toggle('fps-method-legacy',legacy);
    bfCard.classList.toggle('fps-method-current',!legacy);
  }
  function applyMaster(){
    const on=master.checked;
    bfCard.classList.toggle('fps-osd-master-off',!on);
    method.classList.toggle('fps-collapsed',!on);
  }
  sel.addEventListener('change',applyMethod);
  master.addEventListener('change',applyMaster);
  bf.addEventListener('change',()=>{sel.value=bf.checked?'legacy':'current';applyMaster()});
  applyMethod();
  applyMaster();
}
const css=document.createElement('style');
css.textContent=`
.fps-mode-inactive>.fps-feature-master{display:flex!important}
.fps-osd-method-card{margin:0!important;border:0!important;border-radius:0!important;background:transparent!important;box-shadow:none!important}
.fps-osd-method-card>.fps-osd-method-row{display:flex!important;padding:14px 18px!important;border-bottom:1px solid rgba(124,58,237,.22)}
.fps-osd-method-card.fps-osd-master-off{display:none!important}
.fps-osd-method-card.fps-mode-inactive>.fps-osd-method-row{display:flex!important}
.fps-osd-method-card.fps-mode-inactive>.fps-builder{display:none!important}
.fps-method-legacy>.fps-builder{display:none!important}
.fps-method-legacy>.fps-osd-method-card .fps-builder{display:block!important}
.fps-method-legacy>.fps-osd-method-card .fps-builder.fps-disabled{display:none!important}
#fpsOsdMethod{max-width:330px}
`;
document.head.appendChild(css);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,0));else setTimeout(init,0);
})();