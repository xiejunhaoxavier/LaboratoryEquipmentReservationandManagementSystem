// 前端脚本
let currentUser = null;
let reserveTarget = { id: null, name: '' };
function isLoggedIn(){ return !!(currentUser && typeof currentUser.userId === 'number'); }
function getCurrentUserId(){ return isLoggedIn() ? currentUser.userId : null; }
function isAdmin(){ return !!(currentUser && currentUser.type === 2); }
function isStudent(){ return !!(currentUser && currentUser.type === 0); }

window.addEventListener('error', function(e){
  alert('前端脚本错误：' + (e && e.message ? e.message : '未知错误'));
});

function toTimestamp(dtLocal){ if(!dtLocal) return null; const ms = new Date(dtLocal).getTime(); if(Number.isNaN(ms)) return null; return Math.floor(ms/1000); }
function fmtHM(ts){ const d=new Date(ts*1000); const hh=String(d.getHours()).padStart(2,'0'); const mm=String(d.getMinutes()).padStart(2,'0'); return `${hh}:${mm}`; }
function toLocalISOString(date){ const pad=n=>String(n).padStart(2,'0'); return `${date.getFullYear()}-${pad(date.getMonth()+1)}-${pad(date.getDate())}T${pad(date.getHours())}:${pad(date.getMinutes())}`; }

function setDefaultToolbarTimes(){ const now=new Date(); const plus1h=new Date(now.getTime()+3600*1000); const s=document.getElementById('start'); const e=document.getElementById('end'); if(s) s.value=toLocalISOString(now); if(e) e.value=toLocalISOString(plus1h); }

function openReserve(id,name){ reserveTarget={id,name}; document.getElementById('reserveTitle').textContent=`预约设备：${name}`; const now=new Date(); const plus1h=new Date(now.getTime()+3600*1000); document.getElementById('reserve-start').value=toLocalISOString(now); document.getElementById('reserve-end').value=toLocalISOString(plus1h); const dev=dataLastDeviceMap[id]; const isApply=isStudent() && dev && dev.allowStudent===false; const reasonWrap=document.getElementById('reserve-reason-wrap'); if(reasonWrap) reasonWrap.style.display=isApply?'block':'none'; document.getElementById('reserveModal').style.display='flex'; }

function openExtend(devId,currentEnd){ document.getElementById('extendTitle').textContent=`延长设备 #${devId} 的预约`; const d=new Date(currentEnd*1000); document.getElementById('extend-end').value=toLocalISOString(d); document.getElementById('extendModal').style.display='flex'; reserveTarget={id:devId,name:''}; }

async function confirmExtend(){ const endVal=document.getElementById('extend-end').value; const newEnd=Math.floor(new Date(endVal).getTime()/1000); if(!newEnd||Number.isNaN(newEnd)){ alert('结束时间无效'); return; } const r=await api('/api/extend','POST',{ userId: currentUser.userId, deviceId: reserveTarget.id, endTime: newEnd }); if(r.credit!=null){ currentUser.credit=r.credit; const el=document.getElementById('credit'); if(el) el.textContent=`信用分：${currentUser.credit}`; } alert(r.ok?'延长成功':'延长失败（与后续预约冲突）'); document.getElementById('extendModal').style.display='none'; await refreshAll(); }

async function confirmReserve(){ const startVal=document.getElementById('reserve-start').value; const endVal=document.getElementById('reserve-end').value; let start=Math.floor(new Date(startVal).getTime()/1000); let end=Math.floor(new Date(endVal).getTime()/1000); if(!start||Number.isNaN(start)||!end||Number.isNaN(end)){ const nowSec=Math.floor(Date.now()/1000); start=nowSec; end=nowSec+3600; } if(start>=end){ alert('结束时间必须晚于开始时间'); return; } const dev=reserveTarget; let r; if(isStudent() && dataLastDeviceMap[dev.id] && dataLastDeviceMap[dev.id].allowStudent===false){ const reason=(document.getElementById('reserve-reason')?.value||'').trim(); r=await api('/api/apply','POST',{ userId: currentUser.userId, deviceId: dev.id, startTime: start, endTime: end, reason }); alert(r.ok?'申请已提交，等待管理员审核':'申请失败'); } else { r=await api('/api/reserve','POST',{ userId: currentUser.userId, deviceId: dev.id, startTime: start, endTime: end }); alert(r.ok?'预约成功':'预约失败（时间冲突或信用不足）'); } document.getElementById('reserveModal').style.display='none'; await refreshAll(); }

const BASE='http://localhost:8080';
async function api(path,method='GET',data=null){ const opts={ method, headers:{'Content-Type':'application/json'} }; if(data) opts.body=JSON.stringify(data); try{ const r=await fetch(BASE+path,opts); const text=await r.text(); try{ return JSON.parse(text); } catch{ return { ok:false, message:'响应非JSON', raw:text }; } } catch(e){ return { ok:false, message:e&&e.message?e.message:'网络错误' }; } }

function typeName(t){ return ['耗材','精密','动力'][t]||'未知'; }
function statusName(s){ return ['空闲','已预约','使用中','损坏'][s]||'未知'; }

let dataLastDeviceMap={};
async function loadDevices(){ const data=await api('/api/devices'); const wrap=document.getElementById('devices'); wrap.innerHTML=''; if(!data.ok) return; dataLastDeviceMap={}; data.devices.forEach(dev=>{ dataLastDeviceMap[dev.id]=dev; const card=document.createElement('div'); card.className='card'; const tags=[]; tags.push(`<span class="tag">类型：${typeName(dev.type)}</span>`); tags.push(`<span class="tag">健康：${dev.health}</span>`); tags.push(`<span class="tag">状态：${statusName(dev.status)}</span>`); tags.push(`<span class="tag">学生可预约：${dev.allowStudent ? '是' : '否'}</span>`); if(dev.materialLevel!=null) tags.push(`<span class="tag">材料：${dev.materialLevel.toFixed(1)}%</span>`); if(dev.calibration!=null) tags.push(`<span class="tag">校准：${dev.calibration.toFixed(1)}%</span>`); if(dev.temperature!=null) tags.push(`<span class="tag">温度：${dev.temperature.toFixed(1)}℃</span>`); card.innerHTML=`<div class="name">${dev.name} (#${dev.id})</div>${tags.join(' ')}`;
  const now=Math.floor(Date.now()/1000); const activeList=dev.reservations.filter(r=>now>=r.startTime && now<=r.endTime); if(activeList.length && dev.status!==0){ const info=document.createElement('div'); info.style.marginTop='6px'; if(isAdmin()){ info.innerHTML=activeList.map(r=>`<span class="tag">#${r.userId}：${fmtHM(r.startTime)} - ${fmtHM(r.endTime)}</span>`).join(' '); } else { const mine=activeList.find(r=>r.userId===getCurrentUserId()); if(mine) info.innerHTML=`<span class="tag">时间：${fmtHM(mine.startTime)} - ${fmtHM(mine.endTime)}</span>`; } if(info.innerHTML) card.appendChild(info); }
  const btns=document.createElement('div'); btns.className='row'; const hasMyActive=dev.reservations.some(r=>r.userId===getCurrentUserId() && now>=r.startTime && now<=r.endTime); const myRes=dev.reservations.find(r=>r.userId===getCurrentUserId());
  if(isStudent() && !dev.allowStudent){ const btnApply=document.createElement('button'); btnApply.className='btn btn-primary'; btnApply.textContent='申请'; btnApply.onclick=()=>openReserve(dev.id,dev.name); btns.appendChild(btnApply); } else { const btnReserve=document.createElement('button'); btnReserve.className='btn btn-primary'; btnReserve.textContent='预约'; btnReserve.onclick=()=>openReserve(dev.id,dev.name); btns.appendChild(btnReserve); }
  if(hasMyActive && dev.status===1){ const btnBorrow=document.createElement('button'); btnBorrow.className='btn btn-secondary'; btnBorrow.textContent='借用'; btnBorrow.onclick=async()=>{ const r=await api('/api/borrow','POST',{ userId: currentUser.userId, deviceId: dev.id }); alert(r.ok?'借用成功':'借用失败'); await refreshAll(); }; btns.appendChild(btnBorrow); }
  if(hasMyActive && dev.status===2){ const btnReturn=document.createElement('button'); btnReturn.className='btn btn-danger'; btnReturn.textContent='归还'; btnReturn.onclick=async()=>{ const r=await api('/api/return','POST',{ userId: currentUser.userId, deviceId: dev.id }); if(r.credit!=null){ currentUser.credit=r.credit; const el=document.getElementById('credit'); if(el) el.textContent=`信用分：${currentUser.credit}`; } alert(r.ok?'归还成功（可能因逾期扣分）':'归还失败'); await refreshAll(); }; btns.appendChild(btnReturn); }
  if(myRes){ const btnExtend=document.createElement('button'); btnExtend.className='btn btn-ghost'; btnExtend.textContent='延长预约'; btnExtend.onclick=()=>openExtend(dev.id,myRes.endTime); btns.appendChild(btnExtend); }
  if(isAdmin()){ const btnMaintain=document.createElement('button'); btnMaintain.className='btn btn-ghost'; btnMaintain.textContent='维护设备'; btnMaintain.onclick=async()=>{ const r=await api('/api/admin/maintain','POST',{ deviceId: dev.id }); alert(r.ok?'维护完成':'维护失败'); await refreshAll(); }; btns.appendChild(btnMaintain); }
  if(isAdmin()){ const btnDelete=document.createElement('button'); btnDelete.className='btn btn-danger'; btnDelete.textContent='删除'; btnDelete.onclick=async()=>{ const r=await api('/api/admin/delete','POST',{ deviceId: dev.id }); alert(r.ok?'已删除':'删除失败'); await refreshAll(); }; btns.appendChild(btnDelete); }
  card.appendChild(btns); wrap.appendChild(card); }); }

async function refreshAll(){ await loadDevices(); if(currentUser){ const el=document.getElementById('credit'); if(el) el.textContent=`信用分：${currentUser.credit}`; } }

document.getElementById('refresh').onclick=refreshAll;
document.getElementById('logout').onclick=()=>{ currentUser=null; document.getElementById('loginModal').style.display='flex'; const el=document.getElementById('credit'); if(el) el.textContent='信用分：-'; document.getElementById('addDevice').style.display='none'; };
document.getElementById('addDevice').onclick=async()=>{ const name=prompt('设备名称：'); if(!name) return; const typeStr=prompt('设备类型(0=耗材,1=精密,2=动力)：'); const type=Number(typeStr); if(![0,1,2].includes(type)){ alert('类型输入无效'); return; } const allowStr=prompt('学生可预约？(是/否)：'); const allowStudent=(allowStr||'是').trim()==='是'; const r=await api('/api/admin/add','POST',{ name, type, allowStudent }); alert(r.ok?`已新增设备 #${r.deviceId}`:'新增失败'); await refreshAll(); };

document.getElementById('login').onclick=async()=>{ try{ const username=document.getElementById('username').value.trim(); const password=document.getElementById('password').value.trim(); const r=await api('/api/login','POST',{ username,password }); if(!r || !r.ok){ alert('登录失败'); return; } currentUser={ userId:r.userId, username:r.username, credit:r.credit, priority:r.priority, type:r.type }; document.getElementById('loginModal').style.display='none'; document.getElementById('logout').style.display='inline-block'; if(currentUser.type===2){ document.getElementById('addDevice').style.display='inline-block'; const aBtn=document.getElementById('btnApps'); if(aBtn) aBtn.style.display='inline-block'; } setDefaultToolbarTimes(); await refreshAll(); } catch(e){ alert('登录请求失败，请检查后端是否运行'); } };

document.getElementById('reserveCancel').onclick=()=>{ document.getElementById('reserveModal').style.display='none'; };
document.getElementById('reserveConfirm').onclick=async()=>{ await confirmReserve(); };

document.getElementById('extendCancel').onclick=()=>{ document.getElementById('extendModal').style.display='none'; };
document.getElementById('extendConfirm').onclick=async()=>{ await confirmExtend(); };

// 管理员查看申请入口按钮
if(document.getElementById('toolbar')){ const btnApps=document.createElement('button'); btnApps.className='btn'; btnApps.textContent='查看学生申请'; btnApps.style.display='none'; btnApps.id='btnApps'; document.getElementById('toolbar').appendChild(btnApps); btnApps.onclick=async()=>{ const data=await api('/api/admin/applications'); let modal=document.getElementById('applicationsModal'); if(!modal){ modal=document.createElement('div'); modal.id='applicationsModal'; modal.className='modal-backdrop'; modal.style.display='none'; modal.innerHTML=`<div class="modal"><div style="font-weight:700; margin-bottom:8px">学生预约申请</div><div id="applicationsList" style="max-height:240px; overflow:auto; margin-bottom:8px"></div><div class="row" style="justify-content:flex-end"><button id="appsClose" class="btn btn-ghost">关闭</button></div></div>`; document.body.appendChild(modal); }
  const host=document.getElementById('applicationsList'); host.innerHTML=''; if(data.ok){ data.applications.forEach(a=>{ const row=document.createElement('div'); row.style.marginBottom='8px'; row.innerHTML=`#${a.id} 用户${a.userId} 设备${a.deviceId} 时间 ${fmtHM(a.startTime)} - ${fmtHM(a.endTime)}<br/>原因：${a.reason||''}`; const approve=document.createElement('button'); approve.className='btn btn-primary'; approve.textContent='批准'; approve.onclick=async()=>{ const r=await api('/api/admin/applications/approve','POST',{ appId:a.id }); alert(r.ok?'已批准':'批准失败'); await refreshAll(); document.getElementById('applicationsModal').style.display='none'; }; row.appendChild(approve); host.appendChild(row); }); }
  document.getElementById('applicationsModal').style.display='flex'; const appsCloseEl=document.getElementById('appsClose'); if(appsCloseEl) appsCloseEl.onclick=()=>{ document.getElementById('applicationsModal').style.display='none'; }; } }

// 初次显示登录窗 & 初始化
document.getElementById('loginModal').style.display='flex';
setDefaultToolbarTimes();
setInterval(()=>{ const d=new Date(); const el=document.getElementById('clock'); if(el) el.textContent=`${String(d.getHours()).padStart(2,'0')}:${String(d.getMinutes()).padStart(2,'0')}:${String(d.getSeconds()).padStart(2,'0')}`; },1000);

