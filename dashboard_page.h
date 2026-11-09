#ifndef DASHBOARD_PAGE_H
#define DASHBOARD_PAGE_H

const char DASHBOARD_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Patient Health Monitor</title>
<style>
:root{
  --bg:#eef4ff;--card:#fff;--dark:#0f172a;--muted:#64748b;
  --blue:#2563eb;--green:#16a34a;--red:#ef4444;--orange:#f59e0b;
  --line:#e2e8f0;
}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,Segoe UI,sans-serif;background:linear-gradient(135deg,#eef4ff,#e8f0fe);color:var(--dark);min-height:100vh}
.app{display:flex;min-height:100vh}
.sidebar{width:210px;background:#0f172a;color:#fff;padding:18px 12px;flex-shrink:0}
.brand{font-weight:800;font-size:18px;padding:10px;margin-bottom:18px}
.brand small{display:block;color:#94a3b8;font-size:11px;margin-top:4px}
.nav button{width:100%;border:0;background:transparent;color:#cbd5e1;padding:12px;border-radius:10px;text-align:left;font-weight:700;cursor:pointer;margin-bottom:5px}
.nav button.active,.nav button:hover{background:#2563eb;color:#fff}
.live{margin-top:25px;padding:12px;color:#94a3b8;font-size:12px;border-top:1px solid #334155}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:#16a34a;margin-right:6px}
.dot.off{background:#ef4444}
.main{flex:1;min-width:0}
.topbar{padding:15px 20px;background:#fff;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;align-items:center}
.topbar h1{font-size:20px}.topbar p{font-size:12px;color:var(--muted);margin-top:3px}
.page{display:none;padding:18px;max-width:1200px;margin:auto}.page.active{display:block}
.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:14px}
.card{background:rgba(255,255,255,.96);border:1px solid var(--line);border-radius:16px;padding:18px;box-shadow:0 8px 25px rgba(15,23,42,.07)}
.vital{min-height:145px}
.vital .label{font-size:12px;font-weight:800;color:var(--muted);text-transform:uppercase}
.value{font-size:38px;font-weight:900;margin:12px 0 4px}
.unit{font-size:13px;color:var(--muted);font-weight:700}
.value.tick{animation:tick .35s ease}
@keyframes tick{0%{opacity:.45;transform:scale(.95)}100%{opacity:1;transform:scale(1)}}
.heart{border-top:5px solid #e11d48}.spo2{border-top:5px solid #2563eb}
.temp{border-top:5px solid #f59e0b}.hum{border-top:5px solid #06b6d4}
.badge{display:inline-block;padding:5px 9px;border-radius:999px;font-size:10px;font-weight:900;margin-top:10px}
.good{background:#dcfce7;color:#15803d}.watch{background:#fef3c7;color:#b45309}
.bad{background:#fee2e2;color:#dc2626}.info{background:#dbeafe;color:#1d4ed8}
.section-title{font-size:15px;font-weight:900;margin:18px 0 10px}
.patient-card{display:flex;justify-content:space-between;gap:20px;align-items:center}
.patient-name{font-size:22px;font-weight:900}.chips{display:flex;gap:7px;flex-wrap:wrap;margin-top:8px}
.chip{padding:6px 10px;border-radius:999px;background:#eff6ff;color:#1d4ed8;font-size:12px;font-weight:800}
button.primary,button.danger,button.secondary{border:0;border-radius:9px;padding:10px 14px;font-weight:800;cursor:pointer}
.primary{background:#2563eb;color:#fff}.danger{background:#ef4444;color:#fff}.secondary{background:#e2e8f0;color:#0f172a}
.toolbar{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
select,input,textarea{width:100%;padding:10px;border:1px solid var(--line);border-radius:9px;font:inherit}
.formgrid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}.formgrid label{font-size:12px;font-weight:800;color:var(--muted)}
.formgrid label>*{margin-top:5px}.full{grid-column:1/-1}
table{width:100%;border-collapse:collapse;font-size:12px}th,td{padding:10px;border-bottom:1px solid var(--line);text-align:left}th{background:#f8fafc}
.tablewrap{overflow:auto;max-height:430px}
.alert{padding:14px;border-radius:12px;margin-bottom:9px;border-left:4px solid}.alert h3{font-size:14px;margin-bottom:4px}.alert p{font-size:12px;color:#475569}
.ok{border-color:#16a34a;background:#f0fdf4}.warning{border-color:#f59e0b;background:#fffbeb}.critical{border-color:#ef4444;background:#fef2f2}
@media(max-width:900px){.grid{grid-template-columns:repeat(2,1fr)}.sidebar{width:170px}}
@media(max-width:650px){.app{display:block}.sidebar{width:100%;padding:8px}.nav{display:flex;overflow:auto}.nav button{white-space:nowrap}.live{display:none}.grid,.formgrid{grid-template-columns:1fr}.patient-card{display:block}.topbar{padding:12px}}
</style>
</head>
<body>
<div class="app">
<aside class="sidebar">
  <div class="brand">❤️ Patient Health Monitor<small>ESP32 monitoring system</small></div>
  <div class="nav">
    <button class="active" data-page="dashboard">📊 Dashboard</button>
    <button data-page="patient">👤 Patient</button>
    <button data-page="logging">📋 Logging</button>
    <button data-page="alerts">⚠️ Alerts</button>
  </div>
  <div class="live"><span id="connDot" class="dot"></span><span id="connText">Connecting...</span></div>
</aside>

<main class="main">
<header class="topbar">
  <div><h1 id="pageTitle">Patient Dashboard</h1><p id="pageSubtitle">Real-time health monitoring</p></div>
  <div id="clock">--:--:--</div>
</header>

<section id="page-dashboard" class="page active">
  <div class="card patient-card">
    <div>
      <div class="patient-name" id="dashPatientName">No patient registered</div>
      <div class="chips"><span class="chip" id="dashPatientId">ID: --</span><span class="chip" id="dashBed">Bed: --</span><span class="chip" id="dashWard">Ward: --</span></div>
    </div>
    <span id="sensorStatus" class="badge watch">INITIALIZING</span>
  </div>

  <div class="section-title">Live Vital Signs</div>
  <div class="grid">
    <div class="card vital heart">
      <div class="label">❤️ Heart Rate</div><div class="value" id="heartRate">--</div><div class="unit">BPM</div>
      <span id="heartBadge" class="badge watch">WAITING</span>
    </div>
    <div class="card vital spo2">
      <div class="label">🫁 SpO₂</div><div class="value" id="spo2">--</div><div class="unit">%</div>
      <span id="spo2Badge" class="badge watch">WAITING</span>
    </div>
    <div class="card vital temp">
      <div class="label">🌡️ Room Temperature</div><div class="value" id="roomTemp">--</div><div class="unit">°C</div>
      <span id="tempBadge" class="badge watch">WAITING</span>
    </div>
    <div class="card vital hum">
      <div class="label">💧 Humidity</div><div class="value" id="humidity">--</div><div class="unit">%</div>
      <span id="humBadge" class="badge watch">WAITING</span>
    </div>
  </div>

  <div class="section-title">Sensor Information</div>
  <div class="card">
    <p id="fingerStatus">Place a finger on the MAX30102 sensor to begin measurement.</p>
    <p style="margin-top:8px;color:#64748b;font-size:12px">Room temperature and humidity are measured by the DHT11. The DHT11 is not used for body temperature.</p>
  </div>
</section>

<section id="page-patient" class="page">
  <div class="card">
    <h2>Patient Information</h2>
    <p style="color:#64748b;font-size:12px;margin:5px 0 15px">Enter the patient details used by the dashboard and logging system.</p>
    <form id="patientForm" class="formgrid">
      <label>Patient Name<input id="pName"></label>
      <label>Patient ID<input id="pId"></label>
      <label>Bed<input id="pBed"></label>
      <label>Ward<input id="pWard"></label>
      <label>Age<input id="pAge" type="number"></label>
      <label>Gender<input id="pGender"></label>
      <label>Blood Group<input id="pBlood"></label>
      <label>Contact<input id="pContact"></label>
      <label class="full">Clinical Notes<textarea id="pNotes" rows="4"></textarea></label>
      <div class="full toolbar"><button class="primary" type="submit">Save Patient</button><span id="saveMsg" style="font-size:12px;color:#16a34a"></span></div>
    </form>
  </div>
</section>

<section id="page-logging" class="page">
  <div class="card">
    <div class="toolbar">
      <select id="logInterval" style="width:auto">
        <option value="5000">5 seconds</option><option value="10000">10 seconds</option>
        <option value="30000">30 seconds</option><option value="60000">1 minute</option>
      </select>
      <button id="btnLogStart" class="primary">Start Logging</button>
      <button id="btnLogStop" class="danger">Stop</button>
      <button id="btnLogClear" class="secondary">Clear</button>
      <button id="btnLogExport" class="primary">Export CSV</button>
      <span id="logStatus" class="badge watch">STOPPED</span>
    </div>
  </div>
  <div class="section-title">Recorded Measurements</div>
  <div class="card tablewrap">
    <table><thead><tr><th>#</th><th>Timestamp</th><th>BPM</th><th>SpO₂</th><th>Room Temp °C</th><th>Humidity %</th></tr></thead>
    <tbody id="logBody"><tr><td colspan="6" style="text-align:center;color:#64748b">No records yet.</td></tr></tbody></table>
  </div>
</section>

<section id="page-alerts" class="page">
  <div class="card"><h2>Health Alerts</h2><p style="color:#64748b;font-size:12px;margin-top:5px">Alerts are based on the currently received sensor values.</p></div>
  <div style="margin-top:12px" id="alertsList"></div>
</section>
</main>
</div>

<script>
const $=id=>document.getElementById(id);
const KEY="phm_patient_v2",LOGKEY="phm_log_v2";
let patient=JSON.parse(localStorage.getItem(KEY)||'{"name":"","patientId":"","bed":"","ward":"","age":"","gender":"","bloodGroup":"","contact":"","notes":""}');
let latest={heartRate:-1,spo2:-1,roomT:0,hum:0,finger:false,measuring:false,spo2Valid:false,max:0};
let logs=JSON.parse(localStorage.getItem(LOGKEY)||"[]"),timer=null;

function clock(){ $("clock").textContent=new Date().toLocaleTimeString(); }
setInterval(clock,1000);clock();

function loadPatient(){
 $("pName").value=patient.name||"";$("pId").value=patient.patientId||"";$("pBed").value=patient.bed||"";
 $("pWard").value=patient.ward||"";$("pAge").value=patient.age||"";$("pGender").value=patient.gender||"";
 $("pBlood").value=patient.bloodGroup||"";$("pContact").value=patient.contact||"";$("pNotes").value=patient.notes||"";
 $("dashPatientName").textContent=patient.name||"No patient registered";
 $("dashPatientId").textContent="ID: "+(patient.patientId||"--");
 $("dashBed").textContent="Bed: "+(patient.bed||"--");
 $("dashWard").textContent="Ward: "+(patient.ward||"--");
}
loadPatient();

document.querySelectorAll(".nav button").forEach(b=>b.onclick=()=>{
 document.querySelectorAll(".nav button").forEach(x=>x.classList.remove("active"));
 document.querySelectorAll(".page").forEach(x=>x.classList.remove("active"));
 b.classList.add("active");$("page-"+b.dataset.page).classList.add("active");
 $("pageTitle").textContent={dashboard:"Patient Dashboard",patient:"Patient Information",logging:"Patient Logging",alerts:"Health Alerts"}[b.dataset.page];
});

$("patientForm").onsubmit=e=>{
 e.preventDefault();
 patient={name:$("pName").value,patientId:$("pId").value,bed:$("pBed").value,ward:$("pWard").value,
 age:$("pAge").value,gender:$("pGender").value,bloodGroup:$("pBlood").value,contact:$("pContact").value,notes:$("pNotes").value};
 localStorage.setItem(KEY,JSON.stringify(patient));loadPatient();$("saveMsg").textContent="Patient information saved.";
 setTimeout(()=>$("saveMsg").textContent="",2000);
};

function badge(el,text,cls){el.textContent=text;el.className="badge "+cls}
function setVal(id,text){const el=$(id);text=String(text);if(el.textContent!==text){el.textContent=text;el.classList.remove("tick");void el.offsetWidth;el.classList.add("tick");}}
function updateUI(d){
 latest={heartRate:Number(d.heartRate),spo2:Number(d.spo2),roomT:Number(d.roomT),hum:Number(d.hum),finger:!!d.finger,measuring:!!d.measuring,spo2Valid:!!d.spo2Valid,max:Number(d.max)};
 setVal("heartRate",latest.heartRate>0?latest.heartRate:"--");
 setVal("spo2",latest.spo2>0?latest.spo2:"--");
 $("roomTemp").textContent=Number.isFinite(latest.roomT)?latest.roomT.toFixed(1):"--";
 $("humidity").textContent=Number.isFinite(latest.hum)?latest.hum.toFixed(1):"--";
 $("fingerStatus").textContent=!latest.finger?"Place a finger on the MAX30102 sensor to begin measurement.":(latest.measuring?"Finger detected — measuring, keep still for a few seconds...":"Finger detected — live readings from MAX30102.");
 badge($("heartBadge"),latest.heartRate>0?"LIVE":(latest.finger&&latest.measuring?"MEASURING":"WAITING"),latest.heartRate>0?"good":"watch");
 badge($("spo2Badge"),latest.spo2Valid&&latest.spo2>0?"LIVE":(latest.finger?"MEASURING":"WAITING"),latest.spo2Valid?"good":"watch");
 badge($("tempBadge"),"DHT11", "info");badge($("humBadge"),"DHT11","info");
 badge($("sensorStatus"),latest.max?"MAX30102 OK":"MAX30102 ERROR",latest.max?"good":"bad");
 updateAlerts();
}
let polling=false;
async function poll(){
 if(polling)return;polling=true;
 try{
  const r=await fetch("/api/vitals",{cache:"no-store"});if(!r.ok)throw Error();
  updateUI(await r.json());$("connDot").className="dot";$("connText").textContent="Connected";
 }catch(e){$("connDot").className="dot off";$("connText").textContent="Disconnected";}
 polling=false;
}
setInterval(poll,500);poll();

function updateAlerts(){
 let a=[];
 if(!latest.finger)a.push(["warning","No finger detected","Place the patient's finger correctly on the MAX30102 sensor."]);
 if(latest.spo2>0&&latest.spo2<94)a.push(["critical","Low SpO₂ reading","Verify sensor placement and recheck the patient."]);
 if(latest.heartRate>0&&(latest.heartRate<50||latest.heartRate>120))a.push(["warning","Heart rate outside configured range","Review the reading and assess the patient as appropriate."]);
 if(!a.length)a.push(["ok","No active alerts","Current monitored values do not trigger the configured dashboard alerts."]);
 $("alertsList").innerHTML=a.map(x=>`<div class="alert ${x[0]}"><h3>${x[1]}</h3><p>${x[2]}</p></div>`).join("");
}

function renderLogs(){
 $("logBody").innerHTML=logs.length?logs.slice().reverse().map((r,i)=>`<tr><td>${logs.length-i}</td><td>${r.ts}</td><td>${r.bpm>0?r.bpm:"--"}</td><td>${r.spo2>0?r.spo2+"%":"--"}</td><td>${r.roomT.toFixed(1)}</td><td>${r.hum.toFixed(1)}</td></tr>`).join(""):'<tr><td colspan="6" style="text-align:center;color:#64748b">No records yet.</td></tr>';
}
function record(){logs.push({ts:new Date().toLocaleString(),bpm:latest.heartRate,spo2:latest.spo2,roomT:latest.roomT,hum:latest.hum});if(logs.length>500)logs.shift();localStorage.setItem(LOGKEY,JSON.stringify(logs));renderLogs();}
function startLog(){if(timer)return;timer=setInterval(record,Number($("logInterval").value));badge($("logStatus"),"RUNNING","good");}
function stopLog(){clearInterval(timer);timer=null;badge($("logStatus"),"STOPPED","watch");}
$("btnLogStart").onclick=startLog;$("btnLogStop").onclick=stopLog;
$("btnLogClear").onclick=()=>{if(confirm("Clear all logged data?")){logs=[];localStorage.setItem(LOGKEY,"[]");renderLogs();}};
$("btnLogExport").onclick=()=>{
 const rows=[["Timestamp","Heart Rate (BPM)","SpO2 (%)","Room Temperature (C)","Humidity (%)"],...logs.map(r=>[r.ts,r.bpm>0?r.bpm:"",r.spo2>0?r.spo2:"",r.roomT.toFixed(1),r.hum.toFixed(1)])];
 const csv=rows.map(r=>r.map(v=>`"${String(v).replace(/"/g,'""')}"`).join(",")).join("\r\n");
 const a=document.createElement("a");a.href=URL.createObjectURL(new Blob([csv],{type:"text/csv"}));a.download="PHM_Vitals.csv";a.click();
};
renderLogs();
</script>
</body>
</html>
)rawliteral";

#endif