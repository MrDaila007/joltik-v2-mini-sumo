// Embedded Web UI for Joltik V2 Web Controller
// Served from PROGMEM to save RAM

#ifndef WEB_UI_H
#define WEB_UI_H

const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Joltik V2</title>
<style>
:root{--bg:#0d1117;--panel:#161b22;--border:#30363d;--accent:#58a6ff;--danger:#f85149;
--ok:#3fb950;--warn:#d29922;--text:#c9d1d9;--dim:#8b949e;--input:#0d1117}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Courier New',monospace;background:var(--bg);color:var(--text);
min-height:100vh;max-width:600px;margin:0 auto}
.hdr{display:flex;justify-content:space-between;align-items:center;padding:12px 16px;
background:var(--panel);border-bottom:1px solid var(--border)}
.hdr h1{font-size:16px;color:var(--accent);letter-spacing:1px}
.conn{display:flex;align-items:center;gap:6px;font-size:12px;color:var(--dim)}
.dot{width:8px;height:8px;border-radius:50%;background:var(--danger);transition:.3s}
.dot.on{background:var(--ok)}
.tabs{display:flex;background:var(--panel);border-bottom:1px solid var(--border)}
.tab{flex:1;padding:10px 4px;text-align:center;cursor:pointer;border:none;
background:none;color:var(--dim);font-family:inherit;font-size:12px;
border-bottom:2px solid transparent;transition:.2s}
.tab.act{color:var(--accent);border-bottom-color:var(--accent)}
.tab:active{opacity:.7}
.pnl{display:none;padding:16px}
.pnl.act{display:block}
.card{background:var(--panel);border:1px solid var(--border);border-radius:8px;
padding:14px;margin-bottom:12px}
.card h3{font-size:13px;color:var(--accent);margin-bottom:10px;text-transform:uppercase;
letter-spacing:.5px}
.state-badge{display:inline-block;padding:4px 12px;border-radius:4px;font-size:12px;
font-weight:bold;background:var(--border);margin-bottom:8px}
.state-WAITING{background:#1f3a5f;color:var(--accent)}
.state-SEARCHING{background:#1a4023;color:var(--ok)}
.state-ATTACKING{background:#4a1d1d;color:var(--danger)}
.state-AVOIDING{background:#3d2e0f;color:var(--warn)}
/* Robot visualization */
.robot-vis{position:relative;width:180px;height:200px;margin:0 auto 12px}
.robot-body{position:absolute;top:40px;left:30px;width:120px;height:130px;
border:2px solid var(--border);border-radius:12px 12px 8px 8px;background:#1a2233}
.robot-body::after{content:'JOLTIK';position:absolute;top:50%;left:50%;
transform:translate(-50%,-50%);font-size:10px;color:var(--dim);letter-spacing:2px}
.sen{position:absolute;width:18px;height:18px;border-radius:50%;border:2px solid var(--border);
background:#1a1a2e;transition:.3s;font-size:8px;display:flex;align-items:center;
justify-content:center;color:var(--dim)}
.sen.on{background:var(--danger);border-color:var(--danger);color:#fff;box-shadow:0 0 8px var(--danger)}
.sen-fr{top:18px;right:38px}.sen-fl{top:18px;left:38px}
.sen-r{top:90px;right:8px}.sen-l{top:90px;left:8px}
.line-wrap{display:flex;gap:16px;justify-content:center}
.line-bar{flex:1;max-width:120px}
.line-bar label{display:block;font-size:11px;color:var(--dim);margin-bottom:4px;text-align:center}
.line-track{height:8px;background:var(--border);border-radius:4px;overflow:hidden}
.line-fill{height:100%;background:var(--warn);border-radius:4px;transition:.15s;width:0%}
.line-val{font-size:11px;text-align:center;margin-top:2px;color:var(--dim)}
/* Config */
.cfg-row{display:flex;align-items:center;justify-content:space-between;padding:8px 0;
border-bottom:1px solid var(--border)}
.cfg-row:last-child{border-bottom:none}
.cfg-row label{font-size:12px;color:var(--dim)}
.cfg-row input,.cfg-row select{background:var(--input);border:1px solid var(--border);
color:var(--text);padding:6px 10px;border-radius:4px;font-family:inherit;font-size:13px;
width:140px;text-align:right}
.cfg-row select{width:156px}
.toggle{position:relative;width:44px;height:24px;cursor:pointer}
.toggle input{display:none}
.toggle span{position:absolute;inset:0;background:var(--border);border-radius:12px;transition:.3s}
.toggle span::after{content:'';position:absolute;top:3px;left:3px;width:18px;height:18px;
background:var(--dim);border-radius:50%;transition:.3s}
.toggle input:checked+span{background:var(--ok)}
.toggle input:checked+span::after{transform:translateX(20px);background:#fff}
.btn{padding:8px 16px;border:1px solid var(--border);border-radius:6px;cursor:pointer;
font-family:inherit;font-size:12px;background:var(--panel);color:var(--text);transition:.2s}
.btn:active{transform:scale(.96)}
.btn-accent{background:var(--accent);color:#000;border-color:var(--accent);font-weight:bold}
.btn-danger{background:var(--danger);color:#fff;border-color:var(--danger);font-weight:bold}
.btn-ok{background:var(--ok);color:#000;border-color:var(--ok);font-weight:bold}
.btn-row{display:flex;gap:8px;margin-top:10px;flex-wrap:wrap}
/* Test controls */
.motor-ctrl{display:flex;gap:16px;align-items:center;justify-content:center;margin:8px 0}
.motor-slider{display:flex;flex-direction:column;align-items:center;gap:4px}
.motor-slider label{font-size:11px;color:var(--dim)}
.motor-slider input[type=range]{writing-mode:vertical-lr;direction:rtl;height:120px;
accent-color:var(--accent)}
.motor-slider .val{font-size:14px;font-weight:bold;min-width:40px;text-align:center}
.dpad{display:grid;grid-template-columns:repeat(3,52px);grid-template-rows:repeat(3,44px);
gap:4px;justify-content:center;margin:12px 0}
.dpad .btn{padding:0;display:flex;align-items:center;justify-content:center;font-size:16px}
.dpad .center{border:none;background:none;cursor:default}
.stop-big{width:100%;padding:14px;font-size:16px;margin-top:12px}
/* Debug */
.debug-log{background:#000;border:1px solid var(--border);border-radius:6px;
height:50vh;overflow-y:auto;padding:8px;font-size:11px;line-height:1.6;
white-space:pre-wrap;word-break:break-all}
.debug-log .ts{color:var(--dim)}.debug-log .d-msg{color:var(--text)}
.debug-log .d-sensor{color:var(--accent)}.debug-log .d-cmd{color:var(--warn)}
.debug-log .d-err{color:var(--danger)}
.debug-bar{display:flex;gap:8px;margin-bottom:8px;align-items:center}
.debug-bar input{flex:1;background:var(--input);border:1px solid var(--border);color:var(--text);
padding:6px 10px;border-radius:4px;font-family:inherit;font-size:12px}
</style>
</head>
<body>

<div class="hdr">
  <h1>&#9889; JOLTIK V2</h1>
  <div class="conn">
    <span id="ws-status">WS: ---</span>
    <div class="dot" id="ws-dot"></div>
    <span id="bot-status">Bot: ---</span>
    <div class="dot" id="bot-dot"></div>
  </div>
</div>

<div class="tabs">
  <button class="tab act" onclick="showTab('dash')">Dashboard</button>
  <button class="tab" onclick="showTab('cfg')">Config</button>
  <button class="tab" onclick="showTab('test')">Test</button>
  <button class="tab" onclick="showTab('dbg')">Debug</button>
</div>

<!-- Dashboard -->
<div id="dash" class="pnl act">
  <div class="card">
    <h3>Robot State</h3>
    <div id="state-badge" class="state-badge state-WAITING">WAITING</div>
    <div class="robot-vis">
      <div class="robot-body"></div>
      <div class="sen sen-fl" id="s-fl">FL</div>
      <div class="sen sen-fr" id="s-fr">FR</div>
      <div class="sen sen-l" id="s-l">L</div>
      <div class="sen sen-r" id="s-r">R</div>
    </div>
    <div class="line-wrap">
      <div class="line-bar">
        <label>Line Left</label>
        <div class="line-track"><div class="line-fill" id="line-l-bar"></div></div>
        <div class="line-val" id="line-l-val">0</div>
      </div>
      <div class="line-bar">
        <label>Line Right</label>
        <div class="line-track"><div class="line-fill" id="line-r-bar"></div></div>
        <div class="line-val" id="line-r-val">0</div>
      </div>
    </div>
  </div>
  <div class="btn-row">
    <button class="btn btn-ok" onclick="sendCmd('START')">START</button>
    <button class="btn btn-danger" onclick="sendCmd('STOP')">EMERGENCY STOP</button>
  </div>
</div>

<!-- Config -->
<div id="cfg" class="pnl">
  <div class="card">
    <h3>Robot Configuration</h3>
    <div class="cfg-row">
      <label>Dohyo Command</label>
      <input type="number" id="cfg-dohyo" min="0" max="255" value="18">
    </div>
    <div class="cfg-row">
      <label>Start Mode</label>
      <select id="cfg-mode">
        <option value="0">Face to Face</option>
        <option value="1">Back to Back</option>
      </select>
    </div>
    <div class="cfg-row">
      <label>Line Sensors</label>
      <label class="toggle">
        <input type="checkbox" id="cfg-line" checked>
        <span></span>
      </label>
    </div>
    <div class="btn-row">
      <button class="btn btn-accent" onclick="saveConfig()">Save to Robot</button>
      <button class="btn" onclick="sendCmd('GETCFG')">Reload</button>
    </div>
  </div>
</div>

<!-- Test -->
<div id="test" class="pnl">
  <div class="card">
    <h3>Motor Control</h3>
    <div class="motor-ctrl">
      <div class="motor-slider">
        <label>Left</label>
        <input type="range" id="m-left" min="-255" max="255" value="0"
               oninput="updateMotorVal()">
        <div class="val" id="m-left-val">0</div>
      </div>
      <div class="motor-slider">
        <label>Right</label>
        <input type="range" id="m-right" min="-255" max="255" value="0"
               oninput="updateMotorVal()">
        <div class="val" id="m-right-val">0</div>
      </div>
    </div>
    <div class="btn-row" style="justify-content:center">
      <button class="btn btn-accent" onclick="sendMotors()">Apply</button>
      <button class="btn" onclick="resetSliders()">Reset</button>
    </div>
  </div>
  <div class="card">
    <h3>Quick Actions</h3>
    <div class="dpad">
      <div></div>
      <button class="btn" onmousedown="sendDrive(150,150)" onmouseup="sendDrive(0,0)"
              ontouchstart="sendDrive(150,150)" ontouchend="sendDrive(0,0)">&#9650;</button>
      <div></div>
      <button class="btn" onmousedown="sendDrive(-150,150)" onmouseup="sendDrive(0,0)"
              ontouchstart="sendDrive(-150,150)" ontouchend="sendDrive(0,0)">&#9664;</button>
      <button class="btn center" disabled></button>
      <button class="btn" onmousedown="sendDrive(150,-150)" onmouseup="sendDrive(0,0)"
              ontouchstart="sendDrive(150,-150)" ontouchend="sendDrive(0,0)">&#9654;</button>
      <div></div>
      <button class="btn" onmousedown="sendDrive(-150,-150)" onmouseup="sendDrive(0,0)"
              ontouchstart="sendDrive(-150,-150)" ontouchend="sendDrive(0,0)">&#9660;</button>
      <div></div>
    </div>
    <button class="btn btn-danger stop-big" onclick="sendCmd('STOP')">STOP</button>
  </div>
</div>

<!-- Debug -->
<div id="dbg" class="pnl">
  <div class="debug-bar">
    <input type="text" id="dbg-filter" placeholder="Filter..." oninput="filterDebug()">
    <button class="btn" onclick="clearDebug()">Clear</button>
    <label style="font-size:11px;color:var(--dim);white-space:nowrap">
      <input type="checkbox" id="dbg-auto" checked> Auto-scroll
    </label>
  </div>
  <div class="debug-log" id="dbg-log"></div>
</div>

<script>
// ---- WebSocket ----
let ws = null;
let reconnectTimer = null;
const MAX_DEBUG_LINES = 500;
let debugLines = [];

function connect() {
  if (ws && ws.readyState <= 1) return;
  ws = new WebSocket('ws://' + location.hostname + ':81/');

  ws.onopen = function() {
    document.getElementById('ws-dot').classList.add('on');
    document.getElementById('ws-status').textContent = 'WS: OK';
    clearTimeout(reconnectTimer);
    addDebug('sys', 'WebSocket connected');
  };

  ws.onclose = function() {
    document.getElementById('ws-dot').classList.remove('on');
    document.getElementById('ws-status').textContent = 'WS: OFF';
    addDebug('sys', 'WebSocket disconnected, reconnecting...');
    reconnectTimer = setTimeout(connect, 2000);
  };

  ws.onerror = function() {
    ws.close();
  };

  ws.onmessage = function(e) {
    handleMessage(e.data);
  };
}

function sendCmd(cmd) {
  if (ws && ws.readyState === 1) {
    ws.send(cmd);
    addDebug('cmd', '> ' + cmd);
  }
}

// ---- Message handling ----
const STATES = ['WAITING','SEARCHING','ATTACKING','AVOIDING'];

function handleMessage(msg) {
  if (msg.startsWith('S:')) {
    // Sensor data: fr,fl,r,l,lineR,lineL,state
    let parts = msg.substring(2).split(',');
    if (parts.length >= 7) {
      updateSensors({
        fr: parts[0]==='1', fl: parts[1]==='1',
        r: parts[2]==='1', l: parts[3]==='1',
        lineR: parseInt(parts[4]), lineL: parseInt(parts[5]),
        state: STATES[parseInt(parts[6])] || 'UNKNOWN'
      });
    }
  }
  else if (msg.startsWith('CFG:')) {
    let parts = msg.substring(4).split(',');
    if (parts.length >= 3) {
      document.getElementById('cfg-dohyo').value = parts[0];
      document.getElementById('cfg-mode').value = parts[1];
      document.getElementById('cfg-line').checked = parts[2]==='1';
      addDebug('sys', 'Config loaded from robot');
    }
  }
  else if (msg.startsWith('CONN:')) {
    let on = msg.charAt(5) === '1';
    document.getElementById('bot-dot').classList.toggle('on', on);
    document.getElementById('bot-status').textContent = on ? 'Bot: OK' : 'Bot: OFF';
  }
  else if (msg === 'OK') {
    addDebug('sys', 'Robot: OK');
  }
  else if (msg.startsWith('D:')) {
    addDebug('msg', msg.substring(2));
  }
}

// ---- Dashboard ----
function updateSensors(d) {
  document.getElementById('s-fr').classList.toggle('on', d.fr);
  document.getElementById('s-fl').classList.toggle('on', d.fl);
  document.getElementById('s-r').classList.toggle('on', d.r);
  document.getElementById('s-l').classList.toggle('on', d.l);

  let maxLine = 1023;
  let lrPct = Math.min(100, d.lineR / maxLine * 100);
  let llPct = Math.min(100, d.lineL / maxLine * 100);
  document.getElementById('line-r-bar').style.width = lrPct + '%';
  document.getElementById('line-l-bar').style.width = llPct + '%';
  document.getElementById('line-r-val').textContent = d.lineR;
  document.getElementById('line-l-val').textContent = d.lineL;

  let badge = document.getElementById('state-badge');
  badge.textContent = d.state;
  badge.className = 'state-badge state-' + d.state;
}

// ---- Config ----
function saveConfig() {
  let dohyo = document.getElementById('cfg-dohyo').value;
  let mode = document.getElementById('cfg-mode').value;
  let line = document.getElementById('cfg-line').checked ? '1' : '0';
  sendCmd('SETCFG:' + dohyo + ',' + mode + ',' + line);
}

// ---- Test controls ----
function updateMotorVal() {
  document.getElementById('m-left-val').textContent = document.getElementById('m-left').value;
  document.getElementById('m-right-val').textContent = document.getElementById('m-right').value;
}

function sendMotors() {
  let l = document.getElementById('m-left').value;
  let r = document.getElementById('m-right').value;
  sendCmd('DRIVE:' + l + ',' + r);
}

function sendDrive(l, r) {
  sendCmd('DRIVE:' + l + ',' + r);
}

function resetSliders() {
  document.getElementById('m-left').value = 0;
  document.getElementById('m-right').value = 0;
  updateMotorVal();
  sendCmd('DRIVE:0,0');
}

// ---- Tabs ----
function showTab(id) {
  document.querySelectorAll('.pnl').forEach(function(p) { p.classList.remove('act'); });
  document.querySelectorAll('.tab').forEach(function(t) { t.classList.remove('act'); });
  document.getElementById(id).classList.add('act');
  // Find matching tab button
  let tabs = document.querySelectorAll('.tab');
  var names = {dash:0, cfg:1, test:2, dbg:3};
  if (names[id] !== undefined) tabs[names[id]].classList.add('act');
}

// ---- Debug console ----
function addDebug(type, text) {
  let now = new Date();
  let ts = pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' + pad(now.getSeconds());
  let cls = 'd-msg';
  if (type === 'cmd') cls = 'd-cmd';
  else if (type === 'sensor') cls = 'd-sensor';
  else if (type === 'err') cls = 'd-err';
  else if (type === 'sys') cls = 'd-sensor';

  debugLines.push({ts: ts, cls: cls, text: text});
  if (debugLines.length > MAX_DEBUG_LINES) debugLines.shift();
  renderDebug();
}

function renderDebug() {
  let filter = document.getElementById('dbg-filter').value.toLowerCase();
  let log = document.getElementById('dbg-log');
  let html = '';
  for (let i = 0; i < debugLines.length; i++) {
    let d = debugLines[i];
    if (filter && d.text.toLowerCase().indexOf(filter) === -1) continue;
    html += '<span class="ts">[' + d.ts + ']</span> <span class="' + d.cls + '">'
          + escHtml(d.text) + '</span>\n';
  }
  log.innerHTML = html;
  if (document.getElementById('dbg-auto').checked) {
    log.scrollTop = log.scrollHeight;
  }
}

function clearDebug() {
  debugLines = [];
  document.getElementById('dbg-log').innerHTML = '';
}

function filterDebug() { renderDebug(); }

function pad(n) { return n < 10 ? '0' + n : '' + n; }

function escHtml(s) {
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

// ---- Touch event fix (prevent double-fire) ----
document.addEventListener('touchstart', function(){}, {passive: true});

// ---- Init ----
connect();
</script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
