#pragma once

// ======================================================================
//  Embedded web dashboard.
//
//  This file is served by the ESP32 from PROGMEM and is ALSO used by
//  simulator/server.js, which extracts the HTML from the C++ raw string
//  literal below. Do not remove or rename that literal!
// ======================================================================

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="dark">
<title>ESP32 Home</title>
<style>
  :root{
    --bg:#0d1117;--panel:#161c26;--panel-2:#1b2330;--border:#242e3d;
    --text:#e8eef5;--muted:#8a97a8;--accent:#3ddc84;--accent-soft:rgba(61,220,132,.14);
    --warn:#f0a04b;--danger:#f85149;--radius:14px;
  }
  *{box-sizing:border-box;margin:0;padding:0}
  body{
    font-family:system-ui,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
    background:radial-gradient(1100px 550px at 85% -10%,rgba(61,220,132,.07),transparent),var(--bg);
    color:var(--text);display:flex;flex-direction:column;min-height:100vh;
  }
  header{display:flex;align-items:center;justify-content:space-between;gap:12px;
    padding:18px 22px;border-bottom:1px solid var(--border);flex-wrap:wrap}
  .brand{display:flex;align-items:center;gap:12px}
  .logo{width:40px;height:40px;border-radius:12px;display:grid;place-items:center;font-size:20px;
    background:linear-gradient(135deg,#3ddc84,#178a4d);flex:none}
  h1{font-size:1.1rem;font-weight:650;letter-spacing:.2px}
  .sub{font-size:.72rem;color:var(--muted);margin-top:2px}
  .conn{display:flex;align-items:center;gap:8px;font-size:.8rem;color:var(--muted);
    background:var(--panel);border:1px solid var(--border);padding:7px 12px;border-radius:999px}
  .dot{width:9px;height:9px;border-radius:50%;background:var(--danger);flex:none}
  .conn.ok .dot{background:var(--accent);box-shadow:0 0 0 3px var(--accent-soft)}
  main{flex:1;width:100%;max-width:1080px;margin:0 auto;padding:22px}
  .section-title{font-size:.72rem;text-transform:uppercase;letter-spacing:.12em;color:var(--muted);margin:6px 2px 12px}
  .tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin-bottom:26px}
  .tile{background:var(--panel);border:1px solid var(--border);border-radius:var(--radius);
    padding:16px 16px 14px;transition:border-color .3s}
  .tile .label{font-size:.72rem;color:var(--muted);display:flex;justify-content:space-between}
  .tile .value{font-size:1.65rem;font-weight:650;margin-top:6px;font-variant-numeric:tabular-nums}
  .tile .unit{font-size:.9rem;color:var(--muted);font-weight:400}
  .tile.alert{border-color:var(--warn)}
  .tile.alert .value{color:var(--warn)}
  .cards{display:grid;grid-template-columns:repeat(auto-fill,minmax(255px,1fr));gap:12px}
  .card{background:var(--panel);border:1px solid var(--border);border-radius:var(--radius);
    padding:16px;display:flex;align-items:center;gap:14px;transition:border-color .25s,background .25s}
  .card.on{border-color:rgba(61,220,132,.45);background:var(--panel-2)}
  .icon{width:44px;height:44px;border-radius:12px;background:var(--panel-2);
    display:grid;place-items:center;font-size:22px;flex:none}
  .card.on .icon{background:var(--accent-soft)}
  .info{flex:1;min-width:0}
  .name{font-weight:600;font-size:.95rem;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  .state{font-size:.75rem;color:var(--muted);margin-top:3px}
  .card.on .state{color:var(--accent)}
  .switch{position:relative;width:52px;height:30px;flex:none;display:inline-block}
  .switch input{opacity:0;width:0;height:0}
  .slider{position:absolute;inset:0;background:#2c3546;border-radius:999px;cursor:pointer;transition:background .25s}
  .slider::before{content:"";position:absolute;width:22px;height:22px;border-radius:50%;left:4px;top:4px;
    background:#fff;transition:transform .25s}
  input:checked + .slider{background:var(--accent)}
  input:checked + .slider::before{transform:translateX(22px)}
  input:disabled + .slider{opacity:.5;cursor:wait}
  footer{display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap;
    padding:16px 22px;border-top:1px solid var(--border);font-size:.75rem;color:var(--muted)}
  .ghost{background:none;border:1px solid var(--border);color:var(--muted);border-radius:999px;
    padding:7px 14px;font-size:.75rem;cursor:pointer;transition:color .2s,border-color .2s}
  .ghost:hover{color:var(--text);border-color:var(--danger)}
  @media (max-width:480px){main{padding:16px}.cards{grid-template-columns:1fr 1fr}}
</style>
</head>
<body>
<header>
  <div class="brand">
    <div class="logo">🏠</div>
    <div>
      <h1 id="deviceName">ESP32 Home</h1>
      <div class="sub" id="deviceMeta">connecting&hellip;</div>
    </div>
  </div>
  <div class="conn" id="conn"><span class="dot"></span><span id="connText">Connecting</span></div>
</header>

<main>
  <div class="section-title">Environment</div>
  <div class="tiles">
    <div class="tile"><div class="label"><span>Temperature</span><span>🌡️</span></div>
      <div class="value"><span id="temp">--</span> <span class="unit">°C</span></div></div>
    <div class="tile"><div class="label"><span>Humidity</span><span>💧</span></div>
      <div class="value"><span id="hum">--</span> <span class="unit">%</span></div></div>
    <div class="tile" id="motionTile"><div class="label"><span>Motion</span><span>🏃</span></div>
      <div class="value" id="motion">--</div></div>
    <div class="tile"><div class="label"><span>Uptime</span><span>⏱️</span></div>
      <div class="value" id="uptime" style="font-size:1.1rem">--</div></div>
  </div>

  <div class="section-title">Controls</div>
  <div class="cards" id="cards"></div>
</main>

<footer>
  <span id="stats">ESP32 Home Automation</span>
  <button class="ghost" onclick="allOff()">Turn all off</button>
</footer>

<script>
(function(){
  "use strict";
  var $=function(id){return document.getElementById(id)};
  var cards=$("cards"), built=-1, pending={};

  // pick a fitting icon from the channel name
  var ICONS=[[/(fan|cool)/i,"\uD83C\uDF00"],[/(light|lamp|bulb)/i,"\uD83D\uDCA1"],
             [/(heat|warm)/i,"\uD83D\uDD25"],[/(pump|water|irrig)/i,"\uD83D\uDCA7"],
             [/(door|garage|gate)/i,"\uD83D\uDEAA"],[/(curtain|blind)/i,"\uD83E\uDE9F"],
             [/(tv|media|audio)/i,"\uD83D\uDCFA"],[/(lock)/i,"\uD83D\uDD10"]];
  function icon(name){
    for(var i=0;i<ICONS.length;i++){if(ICONS[i][0].test(name)){return ICONS[i][1]}}
    return "\uD83D\uDD0C";
  }

  function fmtUptime(s){
    var d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60),sec=s%60,out="";
    if(d){out+=d+"d "}if(d||h){out+=h+"h "}if(d||h||m){out+=m+"m "}
    return out+sec+"s";
  }

  function build(relays){
    cards.innerHTML="";
    relays.forEach(function(r,i){
      var card=document.createElement("div");
      card.className="card";card.id="card"+i;
      card.innerHTML='<div class="icon">'+icon(r.name)+'</div>'+
        '<div class="info"><div class="name"></div><div class="state"></div></div>'+
        '<label class="switch"><input type="checkbox"><span class="slider"></span></label>';
      card.querySelector(".name").textContent=r.name;
      var input=card.querySelector("input");
      input.addEventListener("change",function(){toggle(i,input)});
      cards.appendChild(card);
    });
    built=relays.length;
  }

  function setCard(ch,on){
    var card=$("card"+ch);
    if(!card){return}
    card.classList.toggle("on",on);
    card.querySelector(".state").textContent=on?"On":"Off";
  }

  function render(s){
    document.title=s.device;
    $("deviceName").textContent=s.device;
    var sens=s.sensors||{};
    $("temp").textContent=(sens.temperature!=null)?Number(sens.temperature).toFixed(1):"--";
    $("hum").textContent=(sens.humidity!=null)?Number(sens.humidity).toFixed(1):"--";
    var mo=$("motion"),mt=$("motionTile");
    if(sens.motion){mo.textContent="Detected";mt.classList.add("alert")}
    else{mo.textContent="Clear";mt.classList.remove("alert")}
    $("uptime").textContent=fmtUptime(s.uptime||0);
    $("deviceMeta").textContent=(s.ip&&s.ip!=="simulated")?("http://"+s.ip):"device simulator";
    $("stats").textContent=((s.rssi!=null)?("WiFi "+s.rssi+" dBm"):"")+
      (s.freeHeap?(" \u00B7 heap "+Math.round(s.freeHeap/1024)+" kB"):"");
    if(built!==s.relays.length){build(s.relays)}
    s.relays.forEach(function(r,i){
      var card=$("card"+i),input=card.querySelector("input");
      card.classList.toggle("on",r.on);
      card.querySelector(".state").textContent=r.on?"On":"Off";
      if(!pending[i]){input.checked=r.on}
    });
  }

  function setConn(ok,text){
    $("conn").classList.toggle("ok",ok);
    $("connText").textContent=text;
  }

  function api(path,body){
    var opts=body?{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)}:undefined;
    return fetch(path,opts).then(function(r){
      if(!r.ok){throw new Error("HTTP "+r.status)}
      return r.json();
    });
  }

  window.toggle=function(ch,input){
    pending[ch]=true;input.disabled=true;
    var on=input.checked;
    setCard(ch,on);
    api("/api/relay",{channel:ch,state:on})
      .then(function(s){render(s)})
      .catch(function(){input.checked=!on;setCard(ch,!on);setConn(false,"Command failed")})
      .then(function(){delete pending[ch];input.disabled=false});
  };

  window.allOff=function(){
    api("/api/relays",{state:false}).then(render)
      .catch(function(){setConn(false,"Command failed")});
  };

  function poll(){
    api("/api/status")
      .then(function(s){render(s);setConn(true,"Online \u00B7 "+(s.ip||""))})
      .catch(function(){setConn(false,"Offline \u2014 retrying")});
  }

  poll();
  setInterval(poll,2500);
})();
</script>
</body>
</html>)rawliteral";
