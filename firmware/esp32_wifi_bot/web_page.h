#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

// Pre-compressed / raw string literal of Cockpit Web UI
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>ROBO Race Bot — Cockpit Controller</title>
  <style>
    :root {
      --bg: #0d1117;
      --card-bg: #161b22;
      --border: #30363d;
      --accent: #58a6ff;
      --accent-glow: rgba(88, 166, 255, 0.4);
      --red: #f85149;
      --red-glow: rgba(248, 81, 73, 0.4);
      --green: #2ea043;
      --text: #f0f6fc;
      --text-dim: #8b949e;
    }
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      user-select: none;
      -webkit-user-select: none;
      touch-action: none;
    }
    body {
      background-color: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      overflow: hidden;
      height: 100vh;
      display: flex;
      flex-direction: column;
    }
    header {
      background: var(--card-bg);
      border-bottom: 1px solid var(--border);
      padding: 8px 16px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      flex-shrink: 0;
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 8px;
      font-weight: 800;
      font-size: 1rem;
    }
    .brand span {
      color: var(--accent);
      font-size: 0.75rem;
      border: 1px solid var(--accent);
      padding: 2px 6px;
      border-radius: 4px;
    }
    .telemetry {
      display: flex;
      gap: 14px;
      font-size: 0.8rem;
      align-items: center;
    }
    .badge {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--red);
      box-shadow: 0 0 6px var(--red);
      transition: background 0.3s;
    }
    .badge.connected {
      background: var(--green);
      box-shadow: 0 0 8px var(--green);
    }
    main {
      flex: 1;
      display: grid;
      grid-template-columns: 1fr 220px 1fr;
      padding: 10px;
      gap: 10px;
    }
    @media (max-width: 768px) {
      main {
        grid-template-columns: 1fr 1fr;
        grid-template-rows: auto 1fr;
      }
      .center-panel {
        grid-column: 1 / -1;
        order: -1;
      }
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 10px;
      display: flex;
      flex-direction: column;
      position: relative;
    }
    .card-title {
      font-size: 0.75rem;
      text-transform: uppercase;
      letter-spacing: 1px;
      color: var(--text-dim);
      margin-bottom: 6px;
      text-align: center;
    }
    .joystick-container {
      flex: 1;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    canvas.joystick-canvas {
      width: 100%;
      height: 100%;
      max-width: 240px;
      max-height: 240px;
      border-radius: 50%;
      background: radial-gradient(circle, rgba(88,166,255,0.05) 0%, rgba(22,27,34,0.8) 70%);
      border: 2px dashed rgba(255,255,255,0.1);
    }
    .center-panel {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .motor-gauges {
      display: flex;
      justify-content: space-around;
      background: rgba(0,0,0,0.3);
      border-radius: 8px;
      padding: 8px;
    }
    .gauge-col {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      font-size: 0.75rem;
      color: var(--text-dim);
    }
    .gauge-bar-wrapper {
      width: 22px;
      height: 80px;
      background: #21262d;
      border-radius: 8px;
      position: relative;
      overflow: hidden;
      display: flex;
      flex-direction: column;
      justify-content: flex-end;
    }
    .gauge-bar {
      width: 100%;
      background: linear-gradient(to top, #2ea043, #58a6ff);
      height: 0%;
      transition: height 0.08s ease-out;
    }
    .gauge-val {
      font-weight: bold;
      color: var(--text);
    }
    .mode-group {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 4px;
    }
    .btn-mode {
      background: #21262d;
      border: 1px solid var(--border);
      color: var(--text-dim);
      padding: 6px 2px;
      border-radius: 6px;
      font-size: 0.7rem;
      font-weight: bold;
      cursor: pointer;
      text-align: center;
    }
    .btn-mode.active {
      background: var(--accent);
      color: #fff;
      border-color: var(--accent);
      box-shadow: 0 0 10px var(--accent-glow);
    }
    .btn-mode[data-mode="turbo"].active {
      background: var(--red);
      border-color: var(--red);
      box-shadow: 0 0 10px var(--red-glow);
    }
    .action-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 6px;
    }
    .btn-action {
      background: #21262d;
      border: 1px solid var(--border);
      color: var(--text);
      padding: 8px;
      border-radius: 6px;
      font-size: 0.75rem;
      font-weight: 600;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 4px;
    }
    .btn-action.active {
      background: #388bfd33;
      border-color: var(--accent);
      color: var(--accent);
    }
    .btn-stop {
      grid-column: 1 / -1;
      background: var(--red);
      color: white;
      border: none;
      font-weight: 800;
      padding: 10px;
      border-radius: 6px;
    }
    .slider-group {
      display: flex;
      flex-direction: column;
      gap: 2px;
      font-size: 0.7rem;
      color: var(--text-dim);
    }
    input[type=range] {
      width: 100%;
      accent-color: var(--accent);
    }
    footer {
      background: #090d13;
      border-top: 1px solid var(--border);
      padding: 6px 12px;
      font-size: 0.7rem;
      color: var(--text-dim);
      display: flex;
      justify-content: space-between;
      flex-shrink: 0;
    }
  </style>
</head>
<body>
  <header>
    <div class="brand">🏎️ ROBO RACE BOT <span>PRO</span></div>
    <div class="telemetry">
      <div style="display:flex; align-items:center; gap:6px;">
        <span class="badge" id="statusBadge"></span>
        <span id="statusText">Connecting...</span>
      </div>
      <div>Bat: <strong id="batteryVal">-- V</strong></div>
      <div>Ping: <strong id="pingVal">-- ms</strong></div>
    </div>
  </header>

  <main>
    <section class="card">
      <div class="card-title">Steering / Throttle Vector</div>
      <div class="joystick-container">
        <canvas id="joystickCanvas" class="joystick-canvas" width="240" height="240"></canvas>
      </div>
      <div style="text-align: center; font-size: 0.75rem; color: var(--text-dim); margin-top: 4px;">
        X: <span id="valX">0</span>% | Y: <span id="valY">0</span>%
      </div>
    </section>

    <section class="card center-panel">
      <div class="card-title">Cockpit Controls</div>
      <div class="motor-gauges">
        <div class="gauge-col">
          <div class="gauge-bar-wrapper"><div class="gauge-bar" id="motorLeftBar"></div></div>
          <span class="gauge-val" id="motorLeftVal">0%</span>
          <span>LEFT</span>
        </div>
        <div class="gauge-col">
          <div class="gauge-bar-wrapper"><div class="gauge-bar" id="motorRightBar"></div></div>
          <span class="gauge-val" id="motorRightVal">0%</span>
          <span>RIGHT</span>
        </div>
      </div>

      <div class="mode-group">
        <button class="btn-mode" data-mode="eco" onclick="setSpeedMode('eco')">ECO</button>
        <button class="btn-mode active" data-mode="sport" onclick="setSpeedMode('sport')">SPORT</button>
        <button class="btn-mode" data-mode="turbo" onclick="setSpeedMode('turbo')">TURBO</button>
      </div>

      <div class="slider-group">
        <div style="display:flex; justify-content:space-between;">
          <span>Steering Trim</span><span id="trimVal">0</span>
        </div>
        <input type="range" min="-30" max="30" value="0" oninput="updateTrim(this.value)">
      </div>

      <div class="action-grid">
        <button class="btn-action" id="btnLight" onclick="toggleHeadlight()">💡 Light</button>
        <button class="btn-action" id="btnHorn" onmousedown="triggerHorn(true)" onmouseup="triggerHorn(false)" ontouchstart="triggerHorn(true)" ontouchend="triggerHorn(false)">📢 Horn</button>
        <button class="btn-action btn-stop" onclick="emergencyBrake()">🛑 EMERGENCY BRAKE</button>
      </div>
    </section>

    <section class="card">
      <div class="card-title">Live Diagnostics</div>
      <div style="flex:1; display:flex; flex-direction:column; justify-content:center; gap:10px; font-size:0.8rem;">
        <div style="display:flex; justify-content:space-between; border-bottom:1px solid var(--border); padding-bottom:4px;">
          <span style="color:var(--text-dim)">Control Mode</span>
          <strong id="activeInputMode">Touch / Web</strong>
        </div>
        <div style="display:flex; justify-content:space-between; border-bottom:1px solid var(--border); padding-bottom:4px;">
          <span style="color:var(--text-dim)">Packet Frequency</span>
          <strong id="hzCounter">0 Hz</strong>
        </div>
        <div style="display:flex; justify-content:space-between; border-bottom:1px solid var(--border); padding-bottom:4px;">
          <span style="color:var(--text-dim)">Gamepad API</span>
          <strong id="gamepadVal">Disconnected</strong>
        </div>
        <div style="display:flex; justify-content:space-between;">
          <span style="color:var(--text-dim)">Watchdog Status</span>
          <strong style="color:var(--green)">ARMED (300ms)</strong>
        </div>
      </div>
    </section>
  </main>

  <footer>
    <span>Shortcuts: WASD / Arrows | Space: Brake | Shift: Turbo</span>
    <span>ROBO Race Bot Wireless v2.4</span>
  </footer>

  <script>
    let ws = null;
    let targetX = 0, targetY = 0;
    let speedLimit = 0.8;
    let steeringTrim = 0;
    let headlight = false, horn = false;
    let pingStart = 0, packetsSent = 0, lastHzReset = Date.now();
    let gamepadConnected = false;

    const wsUrl = `ws://${window.location.hostname || "192.168.4.1"}:80/ws`;

    function connectWS() {
      try {
        ws = new WebSocket(wsUrl);
      } catch (e) {}

      ws.onopen = () => {
        document.getElementById("statusBadge").classList.add("connected");
        document.getElementById("statusText").innerText = "CONNECTED";
        document.getElementById("statusText").style.color = "var(--green)";
        measurePing();
      };

      ws.onclose = () => {
        document.getElementById("statusBadge").classList.remove("connected");
        document.getElementById("statusText").innerText = "OFFLINE";
        document.getElementById("statusText").style.color = "var(--red)";
        setTimeout(connectWS, 1500);
      };

      ws.onmessage = (evt) => {
        try {
          const d = JSON.parse(evt.data);
          if (d.type === "pong") {
            document.getElementById("pingVal").innerText = `${Date.now() - pingStart} ms`;
          } else if (d.type === "telemetry") {
            document.getElementById("batteryVal").innerText = `${(d.battery || 0).toFixed(2)} V`;
          }
        } catch(e) {}
      };
    }

    function measurePing() {
      if (ws && ws.readyState === WebSocket.OPEN) {
        pingStart = Date.now();
        ws.send(JSON.stringify({ type: "ping" }));
      }
      setTimeout(measurePing, 2000);
    }
    connectWS();

    const canvas = document.getElementById("joystickCanvas");
    const ctx = canvas.getContext("2d");
    let isDragging = false;
    const centerX = canvas.width / 2, centerY = canvas.height / 2;
    const maxRadius = canvas.width * 0.38;

    function drawJoystick(x, y) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      ctx.beginPath();
      ctx.arc(centerX, centerY, maxRadius, 0, Math.PI * 2);
      ctx.strokeStyle = "rgba(88, 166, 255, 0.25)";
      ctx.lineWidth = 4;
      ctx.stroke();

      ctx.beginPath();
      ctx.moveTo(centerX, centerY);
      ctx.lineTo(x, y);
      ctx.strokeStyle = "rgba(88, 166, 255, 0.4)";
      ctx.lineWidth = 3;
      ctx.stroke();

      ctx.beginPath();
      ctx.arc(x, y, 24, 0, Math.PI * 2);
      ctx.fillStyle = "#1f6feb";
      ctx.fill();
      ctx.strokeStyle = "#ffffff";
      ctx.lineWidth = 2;
      ctx.stroke();
    }
    drawJoystick(centerX, centerY);

    function updateStickPosition(clientX, clientY) {
      const rect = canvas.getBoundingClientRect();
      let dx = (clientX - rect.left) * (canvas.width / rect.width) - centerX;
      let dy = (clientY - rect.top) * (canvas.height / rect.height) - centerY;
      const dist = Math.hypot(dx, dy);
      if (dist > maxRadius) {
        dx = (dx / dist) * maxRadius;
        dy = (dy / dist) * maxRadius;
      }
      drawJoystick(centerX + dx, centerY + dy);
      targetX = Math.round((dx / maxRadius) * 100);
      targetY = Math.round((-dy / maxRadius) * 100);
      document.getElementById("valX").innerText = targetX;
      document.getElementById("valY").innerText = targetY;
    }

    function resetStick() {
      targetX = 0;
      targetY = 0;
      drawJoystick(centerX, centerY);
      document.getElementById("valX").innerText = 0;
      document.getElementById("valY").innerText = 0;
    }

    canvas.addEventListener("touchstart", (e) => { isDragging = true; updateStickPosition(e.touches[0].clientX, e.touches[0].clientY); });
    window.addEventListener("touchmove", (e) => { if (isDragging && e.touches.length > 0) updateStickPosition(e.touches[0].clientX, e.touches[0].clientY); });
    window.addEventListener("touchend", () => { if (isDragging) { isDragging = false; resetStick(); } });

    canvas.addEventListener("mousedown", (e) => { isDragging = true; updateStickPosition(e.clientX, e.clientY); });
    window.addEventListener("mousemove", (e) => { if (isDragging) updateStickPosition(e.clientX, e.clientY); });
    window.addEventListener("mouseup", () => { if (isDragging) { isDragging = false; resetStick(); } });

    const keysDown = {};
    window.addEventListener("keydown", (e) => {
      keysDown[e.key.toLowerCase()] = true;
      if (e.code === "Space") { emergencyBrake(); e.preventDefault(); }
      calcKeyInput();
    });
    window.addEventListener("keyup", (e) => {
      keysDown[e.key.toLowerCase()] = false;
      if (e.key.toLowerCase() === "h") triggerHorn(false);
      calcKeyInput();
    });

    function calcKeyInput() {
      if (isDragging || gamepadConnected) return;
      let kY = 0, kX = 0;
      if (keysDown["w"] || keysDown["arrowup"]) kY += 100;
      if (keysDown["s"] || keysDown["arrowdown"]) kY -= 100;
      if (keysDown["a"] || keysDown["arrowleft"]) kX -= 100;
      if (keysDown["d"] || keysDown["arrowright"]) kX += 100;
      if (keysDown["shift"]) setSpeedMode("turbo");
      if (keysDown["h"]) triggerHorn(true);
      targetX = kX; targetY = kY;
      drawJoystick(centerX + (targetX/100)*maxRadius, centerY - (targetY/100)*maxRadius);
      document.getElementById("valX").innerText = targetX;
      document.getElementById("valY").innerText = targetY;
    }

    window.addEventListener("gamepadconnected", (e) => {
      gamepadConnected = true;
      document.getElementById("gamepadVal").innerText = "Connected";
      document.getElementById("gamepadVal").style.color = "var(--green)";
    });
    window.addEventListener("gamepaddisconnected", () => {
      gamepadConnected = false;
      document.getElementById("gamepadVal").innerText = "Disconnected";
      document.getElementById("gamepadVal").style.color = "var(--text-dim)";
    });

    function pollGamepad() {
      const gps = navigator.getGamepads ? navigator.getGamepads() : [];
      if (gps && gps[0]) {
        const gp = gps[0];
        let gx = gp.axes[0] || 0, gy = -gp.axes[1] || 0;
        if (Math.abs(gx) < 0.1) gx = 0;
        if (Math.abs(gy) < 0.1) gy = 0;
        if (!isDragging) {
          targetX = Math.round(gx * 100);
          targetY = Math.round(gy * 100);
          drawJoystick(centerX + (targetX/100)*maxRadius, centerY - (targetY/100)*maxRadius);
          document.getElementById("valX").innerText = targetX;
          document.getElementById("valY").innerText = targetY;
        }
      }
      requestAnimationFrame(pollGamepad);
    }
    requestAnimationFrame(pollGamepad);

    function setSpeedMode(mode) {
      document.querySelectorAll(".btn-mode").forEach(b => b.classList.remove("active"));
      document.querySelector(`.btn-mode[data-mode="${mode}"]`).classList.add("active");
      if (mode === "eco") speedLimit = 0.5;
      else if (mode === "sport") speedLimit = 0.8;
      else if (mode === "turbo") speedLimit = 1.0;
    }

    function updateTrim(v) {
      steeringTrim = parseInt(v);
      document.getElementById("trimVal").innerText = (steeringTrim > 0 ? "+" : "") + steeringTrim;
    }

    function toggleHeadlight() {
      headlight = !headlight;
      document.getElementById("btnLight").classList.toggle("active", headlight);
    }

    function triggerHorn(s) {
      horn = s;
      document.getElementById("btnHorn").classList.toggle("active", horn);
    }

    function emergencyBrake() {
      resetStick();
      sendPacket(true);
    }

    function computeMotorSpeeds() {
      let t = targetY * speedLimit;
      let s = (targetX + steeringTrim) * speedLimit;
      let left = Math.max(-100, Math.min(100, Math.round(t + s)));
      let right = Math.max(-100, Math.min(100, Math.round(t - s)));

      const lBar = document.getElementById("motorLeftBar");
      const rBar = document.getElementById("motorRightBar");
      lBar.style.height = Math.abs(left) + "%";
      rBar.style.height = Math.abs(right) + "%";
      lBar.style.background = left < 0 ? "var(--red)" : "linear-gradient(to top, #2ea043, #58a6ff)";
      rBar.style.background = right < 0 ? "var(--red)" : "linear-gradient(to top, #2ea043, #58a6ff)";
      document.getElementById("motorLeftVal").innerText = left + "%";
      document.getElementById("motorRightVal").innerText = right + "%";

      return { l: left, r: right };
    }

    function sendPacket(isBrake = false) {
      const speeds = computeMotorSpeeds();
      const p = {
        type: "ctrl",
        l: isBrake ? 0 : speeds.l,
        r: isBrake ? 0 : speeds.r,
        light: headlight ? 1 : 0,
        horn: horn ? 1 : 0
      };
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(p));
        packetsSent++;
      }
    }

    setInterval(() => sendPacket(), 25); // 40Hz
    setInterval(() => {
      const elapsed = (Date.now() - lastHzReset) / 1000;
      document.getElementById("hzCounter").innerText = `${Math.round(packetsSent / elapsed)} Hz`;
      packetsSent = 0;
      lastHzReset = Date.now();
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
