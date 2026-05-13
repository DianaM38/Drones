#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ─── Configuración WiFi ────────────────────────────────────────────────────
const char* ssid     = "DRONE_AP";
const char* password = "drone1234";
ESP8266WebServer server(80);

// ─── ESCs ─────────────────────────────────────────────────────────────────
Servo esc1, esc2, esc3, esc4;

#define ESC1_PIN D3   // Motor Frontal-Izquierdo
#define ESC2_PIN D4   // Motor Frontal-Derecho
#define ESC3_PIN D5   // Motor Trasero-Izquierdo  (GPIO14)
#define ESC4_PIN D6   // Motor Trasero-Derecho    (GPIO12)

// ─── Límites de potencia ──────────────────────────────────────────────────
#define THROTTLE_MIN   1000   // µs → motores apagados
#define THROTTLE_MAX   1500   // µs → 50 % de potencia máxima
#define THROTTLE_BASE  1200   // µs → hover/idle de vuelo
#define THROTTLE_STEP   80   // µs → incremento por comando

// ─── Estado de motores ────────────────────────────────────────────────────
int m1 = THROTTLE_MIN;   // Frontal-Izquierdo
int m2 = THROTTLE_MIN;   // Frontal-Derecho
int m3 = THROTTLE_MIN;   // Trasero-Izquierdo
int m4 = THROTTLE_MIN;   // Trasero-Derecho

bool armado = false;

// ─── Helpers ──────────────────────────────────────────────────────────────
int clamp(int val) {
  return constrain(val, THROTTLE_MIN, THROTTLE_MAX);
}

void aplicarMotores() {
  esc1.writeMicroseconds(m1);
  esc2.writeMicroseconds(m2);
  esc3.writeMicroseconds(m3);
  esc4.writeMicroseconds(m4);
}

void detenerMotores() {
  m1 = m2 = m3 = m4 = THROTTLE_MIN;
  aplicarMotores();
}

// ─── Movimientos ─────────────────────────────────────────────────────────
//
//         Frontal
//   M1 (FL) ── M2 (FR)
//      |          |
//   M3 (RL) ── M4 (RR)
//         Trasero
//
// Los cuadricópteros en X rotan: M1&M4 giran en un sentido, M2&M3 en el otro.
// Para yaw/roll/pitch se aumenta/reduce el par de motores correspondiente.

void arriba() {        // Aumentar throttle general
  m1 = m2 = m3 = m4 = clamp(m1 + THROTTLE_STEP);
  aplicarMotores();
}

void abajo() {         // Reducir throttle general
  m1 = m2 = m3 = m4 = clamp(m1 - THROTTLE_STEP);
  aplicarMotores();
}

void avanzar() {       // Pitch hacia adelante: bajar motores traseros
  m3 = clamp(m3 + THROTTLE_STEP);
  m4 = clamp(m4 + THROTTLE_STEP);
  aplicarMotores();
}

void retroceder() {    // Pitch hacia atrás: bajar motores frontales
  m1 = clamp(m1 + THROTTLE_STEP);
  m2 = clamp(m2 + THROTTLE_STEP);
  aplicarMotores();
}

void izquierda() {     // Roll izquierda: bajar motores derechos
  m2 = clamp(m2 + THROTTLE_STEP);
  m4 = clamp(m4 + THROTTLE_STEP);
  aplicarMotores();
}

void derecha() {       // Roll derecha: bajar motores izquierdos
  m1 = clamp(m1 + THROTTLE_STEP);
  m3 = clamp(m3 + THROTTLE_STEP);
  aplicarMotores();
}

void yawIzquierda() {  // Yaw anti-horario: M1+M4 vs M2+M3
  m1 = clamp(m1 + THROTTLE_STEP/2);
  m4 = clamp(m4 + THROTTLE_STEP/2);
  m2 = clamp(m2 - THROTTLE_STEP/2);
  m3 = clamp(m3 - THROTTLE_STEP/2);
  aplicarMotores();
}

void yawDerecha() {    // Yaw horario
  m2 = clamp(m2 + THROTTLE_STEP/2);
  m3 = clamp(m3 + THROTTLE_STEP/2);
  m1 = clamp(m1 - THROTTLE_STEP/2);
  m4 = clamp(m4 - THROTTLE_STEP/2);
  aplicarMotores();
}

// ─── Armado / Desarmado ───────────────────────────────────────────────────
void armar() {
  armado = true;
  m1 = m2 = m3 = m4 = THROTTLE_BASE;
  aplicarMotores();
}

void desarmar() {
  armado = false;
  detenerMotores();
}

// ─── Página web ───────────────────────────────────────────────────────────
// (HTML incrustado como string raw — se define en dronePage.h o inline)
extern const char INDEX_HTML[] PROGMEM;

// ─── Rutas HTTP ──────────────────────────────────────────────────────────
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleCmd() {
  if (!server.hasArg("cmd")) { server.send(400, "text/plain", "Sin comando"); return; }
  String cmd = server.arg("cmd");

  if      (cmd == "armar")        armar();
  else if (cmd == "desarmar")     desarmar();
  else if (!armado) { server.send(200, "text/plain", "DESARMADO"); return; }
  else if (cmd == "arriba")       arriba();
  else if (cmd == "abajo")        abajo();
  else if (cmd == "avanzar")      avanzar();
  else if (cmd == "retroceder")   retroceder();
  else if (cmd == "izquierda")    izquierda();
  else if (cmd == "derecha")      derecha();
  else if (cmd == "yaw_izq")      yawIzquierda();
  else if (cmd == "yaw_der")      yawDerecha();
  else if (cmd == "stop")         detenerMotores();

  // Responder con estado actual
  String resp = "{";
  resp += "\"m1\":" + String(m1) + ",";
  resp += "\"m2\":" + String(m2) + ",";
  resp += "\"m3\":" + String(m3) + ",";
  resp += "\"m4\":" + String(m4) + ",";
  resp += "\"armado\":" + String(armado ? "true" : "false");
  resp += "}";
  server.send(200, "application/json", resp);
}

void handleStatus() {
  String resp = "{";
  resp += "\"m1\":" + String(m1) + ",";
  resp += "\"m2\":" + String(m2) + ",";
  resp += "\"m3\":" + String(m3) + ",";
  resp += "\"m4\":" + String(m4) + ",";
  resp += "\"armado\":" + String(armado ? "true" : "false");
  resp += "}";
  server.send(200, "application/json", resp);
}

// ─── Setup ────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Adjuntar ESCs
  esc1.attach(ESC1_PIN, 1000, 2000);
  esc2.attach(ESC2_PIN, 1000, 2000);
  esc3.attach(ESC3_PIN, 1000, 2000);
  esc4.attach(ESC4_PIN, 1000, 2000);

  // Secuencia de armado de ESC (señal mínima 3 s)
  detenerMotores();
  delay(3000);
  Serial.println("ESCs listos");

  // Crear punto de acceso WiFi
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Rutas
  server.on("/",       handleRoot);
  server.on("/cmd",    handleCmd);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

// ─── Loop ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();
}

// ─── HTML embebido ────────────────────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>DRONE CONTROL</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@300;600;700&display=swap');

  :root {
    --bg: #0a0c10;
    --surface: #111520;
    --border: #1e2a3a;
    --accent: #00e5ff;
    --accent2: #ff3d71;
    --warn: #ffaa00;
    --ok: #00e676;
    --text: #c8d8e8;
    --dim: #4a6070;
  }

  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Rajdhani', sans-serif;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 16px;
    gap: 16px;
  }

  /* ── Header ── */
  header {
    width: 100%;
    max-width: 480px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 1px solid var(--border);
    padding-bottom: 10px;
  }
  header h1 {
    font-size: 1.4rem;
    font-weight: 700;
    letter-spacing: .15em;
    color: var(--accent);
    text-transform: uppercase;
  }
  #statusBadge {
    font-family: 'Share Tech Mono', monospace;
    font-size: .75rem;
    padding: 4px 10px;
    border-radius: 20px;
    border: 1px solid var(--accent2);
    color: var(--accent2);
    transition: all .3s;
  }
  #statusBadge.armed {
    border-color: var(--ok);
    color: var(--ok);
    box-shadow: 0 0 8px var(--ok);
  }

  /* ── Arm buttons ── */
  .arm-row {
    display: flex;
    gap: 12px;
    width: 100%;
    max-width: 480px;
  }
  .arm-row button {
    flex: 1;
    padding: 12px;
    border-radius: 6px;
    border: none;
    font-family: 'Rajdhani', sans-serif;
    font-weight: 700;
    font-size: 1rem;
    letter-spacing: .1em;
    cursor: pointer;
    text-transform: uppercase;
    transition: opacity .2s, transform .1s;
  }
  .arm-row button:active { transform: scale(.97); }
  #btnArm   { background: var(--ok);    color: #000; }
  #btnDisarm{ background: var(--accent2); color: #fff; }

  /* ── Motor display ── */
  .motors {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
    width: 100%;
    max-width: 480px;
  }
  .motor-box {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 10px 14px;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }
  .motor-box label {
    font-size: .7rem;
    color: var(--dim);
    letter-spacing: .08em;
    text-transform: uppercase;
    font-family: 'Share Tech Mono', monospace;
  }
  .motor-val {
    font-family: 'Share Tech Mono', monospace;
    font-size: 1.1rem;
    color: var(--accent);
  }
  .motor-bar {
    height: 4px;
    background: var(--border);
    border-radius: 2px;
    overflow: hidden;
  }
  .motor-bar-fill {
    height: 100%;
    background: var(--accent);
    transition: width .3s;
    border-radius: 2px;
  }

  /* ── Control panels ── */
  .panels {
    display: flex;
    flex-direction: column;
    gap: 16px;
    width: 100%;
    max-width: 480px;
  }

  .panel-title {
    font-size: .7rem;
    letter-spacing: .12em;
    color: var(--dim);
    text-transform: uppercase;
    margin-bottom: 6px;
    font-family: 'Share Tech Mono', monospace;
  }

  /* D-pad grid */
  .dpad {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    grid-template-rows: repeat(3, 1fr);
    gap: 6px;
    aspect-ratio: 1;
    max-width: 200px;
    margin: 0 auto;
  }
  .dpad-placeholder { visibility: hidden; }

  /* Altitude column */
  .alt-col {
    display: flex;
    flex-direction: column;
    gap: 6px;
    max-width: 200px;
    margin: 0 auto;
    width: 100%;
  }

  /* Yaw row */
  .yaw-row {
    display: flex;
    gap: 6px;
    justify-content: center;
  }

  .two-col {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    align-items: start;
  }

  /* ── Buttons ── */
  .ctrl-btn {
    background: var(--surface);
    border: 1px solid var(--border);
    color: var(--text);
    border-radius: 8px;
    font-family: 'Share Tech Mono', monospace;
    font-size: .85rem;
    cursor: pointer;
    padding: 14px 8px;
    text-align: center;
    line-height: 1;
    transition: background .15s, border-color .15s, transform .1s, box-shadow .15s;
    user-select: none;
    -webkit-tap-highlight-color: transparent;
  }
  .ctrl-btn:hover { background: #1a2535; border-color: var(--accent); }
  .ctrl-btn:active {
    transform: scale(.93);
    background: #0d1a2a;
    box-shadow: 0 0 10px var(--accent);
    border-color: var(--accent);
  }
  .ctrl-btn.up-down { color: var(--ok); border-color: #1a3a2a; }
  .ctrl-btn.yaw     { color: var(--warn); border-color: #2a2010; }
  .ctrl-btn.stop-btn{
    background: var(--accent2);
    color: #fff;
    border-color: var(--accent2);
    font-size: .8rem;
    padding: 10px;
  }

  /* ── Log ── */
  #log {
    width: 100%;
    max-width: 480px;
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 10px 14px;
    font-family: 'Share Tech Mono', monospace;
    font-size: .72rem;
    color: var(--dim);
    height: 60px;
    overflow-y: auto;
  }
</style>
</head>
<body>

<header>
  <h1>&#x2708; Drone CTL</h1>
  <div id="statusBadge">DESARMADO</div>
</header>

<!-- Arm / Disarm -->
<div class="arm-row">
  <button id="btnArm"    onclick="cmd('armar')">&#9654; Armar</button>
  <button id="btnDisarm" onclick="cmd('desarmar')">&#9646;&#9646; Desarmar</button>
</div>

<!-- Motor readouts -->
<div class="motors">
  <div class="motor-box">
    <label>M1 — FL</label>
    <div class="motor-val" id="v1">1000 µs</div>
    <div class="motor-bar"><div class="motor-bar-fill" id="b1" style="width:0%"></div></div>
  </div>
  <div class="motor-box">
    <label>M2 — FR</label>
    <div class="motor-val" id="v2">1000 µs</div>
    <div class="motor-bar"><div class="motor-bar-fill" id="b2" style="width:0%"></div></div>
  </div>
  <div class="motor-box">
    <label>M3 — RL</label>
    <div class="motor-val" id="v3">1000 µs</div>
    <div class="motor-bar"><div class="motor-bar-fill" id="b3" style="width:0%"></div></div>
  </div>
  <div class="motor-box">
    <label>M4 — RR</label>
    <div class="motor-val" id="v4">1000 µs</div>
    <div class="motor-bar"><div class="motor-bar-fill" id="b4" style="width:0%"></div></div>
  </div>
</div>

<!-- Controls -->
<div class="panels">
  <div class="two-col">

    <!-- D-pad: avanzar / retroceder / izquierda / derecha -->
    <div>
      <div class="panel-title">Dirección</div>
      <div class="dpad">
        <div class="dpad-placeholder"></div>
        <button class="ctrl-btn" ontouchstart="cmd('avanzar')" onclick="cmd('avanzar')">&#9650;<br>FWD</button>
        <div class="dpad-placeholder"></div>

        <button class="ctrl-btn" ontouchstart="cmd('izquierda')" onclick="cmd('izquierda')">&#9664;<br>IZQ</button>
        <button class="ctrl-btn stop-btn" onclick="cmd('stop')">&#9632;<br>STOP</button>
        <button class="ctrl-btn" ontouchstart="cmd('derecha')" onclick="cmd('derecha')">&#9654;<br>DER</button>

        <div class="dpad-placeholder"></div>
        <button class="ctrl-btn" ontouchstart="cmd('retroceder')" onclick="cmd('retroceder')">&#9660;<br>REV</button>
        <div class="dpad-placeholder"></div>
      </div>
    </div>

    <!-- Altitud + Yaw -->
    <div>
      <div class="panel-title">Altitud</div>
      <div class="alt-col">
        <button class="ctrl-btn up-down" ontouchstart="cmd('arriba')" onclick="cmd('arriba')">&#9650;&#9650;<br>SUBIR</button>
        <button class="ctrl-btn up-down" ontouchstart="cmd('abajo')" onclick="cmd('abajo')">&#9660;&#9660;<br>BAJAR</button>
      </div>
      <div class="panel-title" style="margin-top:12px">Yaw</div>
      <div class="yaw-row">
        <button class="ctrl-btn yaw" style="flex:1" onclick="cmd('yaw_izq')">&#8634; L</button>
        <button class="ctrl-btn yaw" style="flex:1" onclick="cmd('yaw_der')">R &#8635;</button>
      </div>
    </div>

  </div>
</div>

<!-- Log -->
<div id="log">Listo.</div>

<script>
  function cmd(c) {
    fetch('/cmd?cmd=' + c)
      .then(r => r.json())
      .then(d => {
        updateMotors(d);
        log('CMD: ' + c + ' | m1=' + d.m1 + ' m2=' + d.m2 + ' m3=' + d.m3 + ' m4=' + d.m4);
      })
      .catch(e => log('ERR: ' + e));
  }

  function updateMotors(d) {
    const fields = ['m1','m2','m3','m4'];
    fields.forEach((m, i) => {
      const val = d[m];
      const pct = ((val - 1000) / 500) * 100; // 1000–1500 → 0–100%
      document.getElementById('v' + (i+1)).textContent = val + ' µs';
      document.getElementById('b' + (i+1)).style.width  = pct + '%';
    });
    const badge = document.getElementById('statusBadge');
    badge.textContent = d.armado ? 'ARMADO' : 'DESARMADO';
    badge.className   = d.armado ? 'armed' : '';
  }

  function log(msg) {
    const el = document.getElementById('log');
    el.textContent = msg;
  }

  // Poll status cada 2 s
  setInterval(() => {
    fetch('/status').then(r => r.json()).then(updateMotors).catch(() => {});
  }, 2000);
</script>
</body>
</html>
)rawliteral";


