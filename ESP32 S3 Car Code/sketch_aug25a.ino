/*
  ESP32-S3 RC Car
  - L298N controls two DC motors
  - Two SG90 servos control camera pan and tilt
  - Buzzer horn
  - Mobile control page at http://192.168.4.1

  Install the "ESP32Servo" library from Arduino IDE Library Manager.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

#if __has_include("esp_arduino_version.h")
#include "esp_arduino_version.h"
#endif
#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

// ---------- Wi-Fi access point ----------
const char *AP_SSID = "RC-Car-S3";
const char *AP_PASSWORD = "DriveSafe1";  // Minimum 8 characters

// ---------- Pins ----------
// L298N: remove the ENA and ENB jumpers to use speed PWM.
constexpr uint8_t IN1 = 4;   // L298N IN1, left motor direction
constexpr uint8_t IN2 = 5;   // L298N IN2, left motor direction
constexpr uint8_t ENA = 6;   // L298N ENA, left motor PWM
constexpr uint8_t IN3 = 7;   // L298N IN3, right motor direction
constexpr uint8_t IN4 = 15;  // L298N IN4, right motor direction
constexpr uint8_t ENB = 16;  // L298N ENB, right motor PWM

constexpr uint8_t PAN_SERVO_PIN = 17;   // Camera pan SG90 signal
constexpr uint8_t TILT_SERVO_PIN = 18;  // Camera tilt SG90 signal
constexpr uint8_t BUZZER_PIN = 21;      // Passive buzzer signal

constexpr uint32_t MOTOR_PWM_FREQUENCY = 1000;
constexpr uint8_t PWM_RESOLUTION = 8;
constexpr uint8_t LEFT_PWM_CHANNEL = 0;
constexpr uint8_t RIGHT_PWM_CHANNEL = 1;
constexpr uint8_t BUZZER_PWM_CHANNEL = 2;
constexpr uint16_t HORN_FREQUENCY = 2200;

constexpr uint32_t MOTOR_TIMEOUT_MS = 450;
constexpr uint32_t HORN_TIMEOUT_MS = 700;
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 15;
constexpr uint8_t SERVO_STEP_DEGREES = 2;

WebServer server(80);
Servo panServo;
Servo tiltServo;

enum DriveState : uint8_t { STOPPED, FORWARD, BACKWARD, LEFT, RIGHT };
DriveState driveState = STOPPED;

uint8_t motorSpeed = 180;
uint8_t panTarget = 90, panCurrent = 90;
uint8_t tiltTarget = 90, tiltCurrent = 90;
bool hornIsOn = false;
uint32_t lastMotorCommand = 0;
uint32_t lastHornCommand = 0;
uint32_t lastPanStep = 0;
uint32_t lastTiltStep = 0;

const char CONTROL_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0c1422">
<title>RC Car S3</title>
<style>
:root{--bg:#0c1422;--card:#172338;--line:rgba(255,255,255,.11);--text:#eff6ff;--muted:#93a3bd;--green:#25d9a6;--red:#ee5266;--blue:#2a405e}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;user-select:none}
body{margin:0;min-height:100vh;background:radial-gradient(circle at 50% -10%,#31557f,#101a2a 48%,#090e17);color:var(--text);font-family:system-ui,-apple-system,Segoe UI,sans-serif;display:flex;justify-content:center}
main{width:min(100%,520px);padding:max(18px,env(safe-area-inset-top)) 18px max(22px,env(safe-area-inset-bottom))}
header{display:flex;justify-content:space-between;align-items:center;margin:5px 2px 18px}h1{font-size:1.25rem;margin:0;letter-spacing:.04em}.green{color:var(--green)}
#connection{font-size:.78rem;color:var(--muted)}.dot{width:9px;height:9px;border-radius:50%;background:#8090a8;display:inline-block;margin-right:7px}.dot.ok{background:var(--green);box-shadow:0 0 12px var(--green)}.dot.bad{background:var(--red)}
.card{background:linear-gradient(145deg,rgba(39,63,97,.94),rgba(18,28,45,.94));border:1px solid var(--line);border-radius:21px;padding:17px;box-shadow:0 16px 40px rgba(0,0,0,.25)}
.top{display:flex;justify-content:space-between;align-items:center}.car{font-size:2.8rem;filter:drop-shadow(0 5px 8px rgba(0,0,0,.28))}.state{text-align:right}.state small,.section-title{display:block;color:var(--muted);font-size:.69rem;letter-spacing:.12em;text-transform:uppercase}.state b{display:block;color:var(--green);font-size:1.06rem;margin-top:4px}
.range-row{margin-top:15px;padding-top:14px;border-top:1px solid var(--line)}.label{display:flex;justify-content:space-between;color:var(--muted);font-size:.76rem;letter-spacing:.08em;text-transform:uppercase}.value{color:var(--green);font-weight:750;font-variant-numeric:tabular-nums}
input[type=range]{width:100%;height:26px;margin:7px 0 -3px;accent-color:var(--green);cursor:pointer}
.section-title{text-align:center;margin:21px 0 10px}.pad{display:grid;grid-template-columns:repeat(3,1fr);grid-template-rows:repeat(3,74px);gap:9px;max-width:310px;margin:auto}
button{border:0;color:var(--text);font:inherit;font-weight:750;cursor:pointer;touch-action:none}.drive{border:1px solid var(--line);border-radius:18px;background:linear-gradient(145deg,#334a6c,#1d2b43);font-size:1.45rem;box-shadow:0 8px 18px rgba(0,0,0,.2)}.drive.pressed,.drive:active{transform:translateY(2px);background:#3b5d88;box-shadow:inset 0 4px 12px rgba(0,0,0,.26)}
#forward{grid-column:2}#left{grid-row:2}#stop{grid-column:2;grid-row:2;background:linear-gradient(145deg,#ff6d7d,#c7334b);border-color:#ff9aa5;font-size:.72rem;letter-spacing:.08em}#right{grid-column:3;grid-row:2}#backward{grid-column:2;grid-row:3}
.camera{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:13px}.mini{border:1px solid var(--line);border-radius:16px;background:rgba(15,25,41,.72);padding:12px}.mini .label{font-size:.66rem}.horn{grid-column:1/-1;min-height:52px;border-radius:16px;background:linear-gradient(145deg,#f5b828,#c47709);border:1px solid #ffd06b;color:#1b1200;letter-spacing:.08em}.horn.pressed,.horn:active{transform:translateY(2px);background:#ffd157}
.hint{color:var(--muted);text-align:center;font-size:.74rem;line-height:1.45;margin:16px 5px 0}.hint kbd{font-family:inherit;background:#0a111c;border:1px solid var(--line);border-radius:4px;padding:1px 4px}.foot{text-align:center;color:#66758d;font-size:.67rem;margin-top:13px}
@media(max-height:650px){main{padding-top:9px}.card{padding:12px}.pad{grid-template-rows:repeat(3,62px)}.section-title{margin:13px 0 8px}.hint{margin-top:11px}}
</style>
</head>
<body>
<main>
  <header><h1>RC CAR <span class="green">S3</span></h1><div><i id="dot" class="dot"></i><span id="connection">Connecting...</span></div></header>
  <section class="card">
    <div class="top"><div class="car">&#128663;</div><div class="state"><small>Drive mode</small><b id="state">STOPPED</b></div></div>
    <div class="range-row"><div class="label"><span>Motor speed</span><output class="value" id="speedValue">71%</output></div><input id="speed" type="range" min="0" max="255" value="180" aria-label="Motor speed"></div>
  </section>

  <div class="section-title">Hold a direction to drive</div>
  <section class="pad" aria-label="Car controls">
    <button class="drive" id="forward" data-dir="forward" aria-label="Forward">&#9650;</button>
    <button class="drive" id="left" data-dir="left" aria-label="Left">&#9664;</button>
    <button class="drive" id="stop" data-dir="stop" aria-label="Stop">STOP</button>
    <button class="drive" id="right" data-dir="right" aria-label="Right">&#9654;</button>
    <button class="drive" id="backward" data-dir="backward" aria-label="Backward">&#9660;</button>
  </section>

  <section class="camera" aria-label="Camera and horn controls">
    <div class="mini"><div class="label"><span>Camera pan</span><output class="value" id="panValue">90&deg;</output></div><input id="pan" type="range" min="0" max="180" value="90" aria-label="Camera pan"></div>
    <div class="mini"><div class="label"><span>Camera tilt</span><output class="value" id="tiltValue">90&deg;</output></div><input id="tilt" type="range" min="0" max="180" value="90" aria-label="Camera tilt"></div>
    <button id="horn" class="horn" aria-label="Hold for horn">HORN</button>
  </section>

  <p class="hint">Camera sliders work independently of driving.<br>Keyboard: <kbd>W</kbd><kbd>A</kbd><kbd>S</kbd><kbd>D</kbd> or arrow keys.</p>
  <div class="foot">A lost connection automatically stops motors and horn.</div>
</main>
<script>
const speed = document.getElementById('speed');
const pan = document.getElementById('pan');
const tilt = document.getElementById('tilt');
const state = document.getElementById('state');
const dot = document.getElementById('dot');
const connection = document.getElementById('connection');
const horn = document.getElementById('horn');

let heldDrive = null, driveTimer = null, hornTimer = null;
let speedSent = Number(speed.value), panSent = Number(pan.value), tiltSent = Number(tilt.value);
let panTimer = null, tiltTimer = null;

function showSpeed(){ document.getElementById('speedValue').textContent = Math.round(Number(speed.value) * 100 / 255) + '%'; }
function showPan(){ document.getElementById('panValue').textContent = pan.value + '\u00b0'; }
function showTilt(){ document.getElementById('tiltValue').textContent = tilt.value + '\u00b0'; }
function showState(value){ state.textContent = value.toUpperCase(); }
function connected(ok){
  dot.className = 'dot ' + (ok ? 'ok' : 'bad');
  connection.textContent = ok ? 'Car connected' : 'Disconnected';
}
async function api(path){
  try {
    const response = await fetch(path, {cache:'no-store'});
    if (!response.ok) throw new Error('request failed');
    connected(true);
    return response.json();
  } catch (error) {
    connected(false);
    throw error;
  }
}
function sendDrive(direction){
  api('/api/drive?dir=' + encodeURIComponent(direction))
    .then(data => showState(data.state)).catch(() => {});
}
function stopDrive(){
  heldDrive = null;
  if (driveTimer) { clearInterval(driveTimer); driveTimer = null; }
  document.querySelectorAll('.drive.pressed').forEach(button => button.classList.remove('pressed'));
  sendDrive('stop');
}
function startDrive(direction, button){
  if (direction === 'stop') { stopDrive(); return; }
  if (heldDrive === direction) return;
  if (driveTimer) clearInterval(driveTimer);
  document.querySelectorAll('.drive.pressed').forEach(item => item.classList.remove('pressed'));
  heldDrive = direction;
  button.classList.add('pressed');
  sendDrive(direction);
  driveTimer = setInterval(() => { if (heldDrive) sendDrive(heldDrive); }, 150);
}
document.querySelectorAll('.drive').forEach(button => {
  const direction = button.dataset.dir;
  button.addEventListener('pointerdown', event => {
    event.preventDefault();
    button.setPointerCapture(event.pointerId);
    startDrive(direction, button);
  });
  ['pointerup','pointercancel','lostpointercapture'].forEach(eventName =>
    button.addEventListener(eventName, () => { if (direction !== 'stop') stopDrive(); })
  );
  button.addEventListener('contextmenu', event => event.preventDefault());
});

speed.addEventListener('input', showSpeed);
speed.addEventListener('change', () => {
  const value = Number(speed.value);
  if (value !== speedSent) { speedSent = value; api('/api/speed?value=' + value).catch(() => {}); }
});
function sendPan(){
  const value = Number(pan.value);
  if (value !== panSent) { panSent = value; api('/api/pan?angle=' + value).catch(() => {}); }
}
function sendTilt(){
  const value = Number(tilt.value);
  if (value !== tiltSent) { tiltSent = value; api('/api/tilt?angle=' + value).catch(() => {}); }
}
pan.addEventListener('input', () => {
  showPan(); clearTimeout(panTimer); panTimer = setTimeout(sendPan, 60);
});
pan.addEventListener('change', () => { clearTimeout(panTimer); sendPan(); });
tilt.addEventListener('input', () => {
  showTilt(); clearTimeout(tiltTimer); tiltTimer = setTimeout(sendTilt, 60);
});
tilt.addEventListener('change', () => { clearTimeout(tiltTimer); sendTilt(); });

function setHorn(on){ api('/api/horn?on=' + (on ? '1' : '0')).catch(() => {}); }
function startHorn(event){
  event.preventDefault();
  if (hornTimer) return;
  horn.classList.add('pressed');
  setHorn(true);
  hornTimer = setInterval(() => setHorn(true), 180);
}
function stopHorn(){
  if (!hornTimer) return;
  clearInterval(hornTimer); hornTimer = null;
  horn.classList.remove('pressed');
  setHorn(false);
}
horn.addEventListener('pointerdown', event => {
  horn.setPointerCapture(event.pointerId);
  startHorn(event);
});
['pointerup','pointercancel','lostpointercapture'].forEach(eventName => horn.addEventListener(eventName, stopHorn));
horn.addEventListener('contextmenu', event => event.preventDefault());

const keys = {ArrowUp:'forward',KeyW:'forward',ArrowDown:'backward',KeyS:'backward',ArrowLeft:'left',KeyA:'left',ArrowRight:'right',KeyD:'right'};
window.addEventListener('keydown', event => {
  const direction = keys[event.code];
  if (!direction || event.repeat) return;
  event.preventDefault();
  startDrive(direction, document.querySelector('[data-dir="' + direction + '"]'));
});
window.addEventListener('keyup', event => {
  if (keys[event.code]) { event.preventDefault(); stopDrive(); }
});
window.addEventListener('blur', () => { stopDrive(); stopHorn(); });
document.addEventListener('visibilitychange', () => { if (document.hidden) { stopDrive(); stopHorn(); } });

api('/api/status').then(data => {
  speed.value = data.speed; speedSent = data.speed;
  pan.value = data.pan; panSent = data.pan;
  tilt.value = data.tilt; tiltSent = data.tilt;
  showSpeed(); showPan(); showTilt(); showState(data.state);
}).catch(() => {});
showSpeed(); showPan(); showTilt();
</script>
</body>
</html>
)HTML";

const char *driveName() {
  switch (driveState) {
    case FORWARD: return "forward";
    case BACKWARD: return "backward";
    case LEFT: return "left";
    case RIGHT: return "right";
    default: return "stopped";
  }
}

void writePwm(uint8_t pin, uint8_t channel, uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(channel, duty);
#endif
}

void setLeftMotor(int8_t direction, uint8_t speed) {
  digitalWrite(IN1, direction > 0 ? HIGH : LOW);
  digitalWrite(IN2, direction < 0 ? HIGH : LOW);
  writePwm(ENA, LEFT_PWM_CHANNEL, direction == 0 ? 0 : speed);
}

void setRightMotor(int8_t direction, uint8_t speed) {
  digitalWrite(IN3, direction > 0 ? HIGH : LOW);
  digitalWrite(IN4, direction < 0 ? HIGH : LOW);
  writePwm(ENB, RIGHT_PWM_CHANNEL, direction == 0 ? 0 : speed);
}

void applyDrive() {
  int8_t leftDirection = 0;
  int8_t rightDirection = 0;
  switch (driveState) {
    case FORWARD:  leftDirection = 1;  rightDirection = 1;  break;
    case BACKWARD: leftDirection = -1; rightDirection = -1; break;
    case LEFT:     leftDirection = -1; rightDirection = 1;  break;
    case RIGHT:    leftDirection = 1;  rightDirection = -1; break;
    case STOPPED: break;
  }
  setLeftMotor(leftDirection, motorSpeed);
  setRightMotor(rightDirection, motorSpeed);
}

void stopMotors() {
  driveState = STOPPED;
  applyDrive();
}

void setHorn(bool enabled) {
  hornIsOn = enabled;
  if (enabled) lastHornCommand = millis();
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(BUZZER_PIN, enabled ? HORN_FREQUENCY : 0);
#else
  ledcWriteTone(BUZZER_PWM_CHANNEL, enabled ? HORN_FREQUENCY : 0);
#endif
}

void updateServoSmoothly(Servo &servo, uint8_t &current, uint8_t target, uint32_t &lastStep) {
  if (current == target || millis() - lastStep < SERVO_STEP_INTERVAL_MS) return;
  const int difference = static_cast<int>(target) - current;
  const uint8_t step = abs(difference) < SERVO_STEP_DEGREES
    ? static_cast<uint8_t>(abs(difference)) : SERVO_STEP_DEGREES;
  current += difference > 0 ? step : -step;
  servo.write(current);
  lastStep = millis();
}

void sendStatus(int code = 200) {
  String json = "{\"state\":\"";
  json += driveName();
  json += "\",\"speed\":";
  json += motorSpeed;
  json += ",\"pan\":";
  json += panTarget;
  json += ",\"tilt\":";
  json += tiltTarget;
  json += ",\"horn\":";
  json += (hornIsOn ? "true" : "false");
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", json);
}

void handleDrive() {
  if (!server.hasArg("dir")) {
    server.send(400, "application/json", "{\"error\":\"missing dir\"}");
    return;
  }
  String direction = server.arg("dir");
  if (direction == "forward") driveState = FORWARD;
  else if (direction == "backward") driveState = BACKWARD;
  else if (direction == "left") driveState = LEFT;
  else if (direction == "right") driveState = RIGHT;
  else if (direction == "stop") driveState = STOPPED;
  else {
    server.send(400, "application/json", "{\"error\":\"invalid direction\"}");
    return;
  }
  if (driveState == STOPPED) stopMotors();
  else {
    lastMotorCommand = millis();
    applyDrive();
  }
  sendStatus();
}

void handleSpeed() {
  if (!server.hasArg("value")) {
    server.send(400, "application/json", "{\"error\":\"missing value\"}");
    return;
  }
  int value = server.arg("value").toInt();
  if (value < 0 || value > 255) {
    server.send(400, "application/json", "{\"error\":\"speed must be 0-255\"}");
    return;
  }
  motorSpeed = static_cast<uint8_t>(value);
  if (driveState != STOPPED) applyDrive();
  sendStatus();
}

bool readAngle(uint8_t &target) {
  if (!server.hasArg("angle")) {
    server.send(400, "application/json", "{\"error\":\"missing angle\"}");
    return false;
  }
  int angle = server.arg("angle").toInt();
  if (angle < 0 || angle > 180) {
    server.send(400, "application/json", "{\"error\":\"angle must be 0-180\"}");
    return false;
  }
  target = static_cast<uint8_t>(angle);
  return true;
}

void handlePan() {
  if (readAngle(panTarget)) sendStatus();
}

void handleTilt() {
  if (readAngle(tiltTarget)) sendStatus();
}

void handleHorn() {
  if (!server.hasArg("on")) {
    server.send(400, "application/json", "{\"error\":\"missing on\"}");
    return;
  }
  String on = server.arg("on");
  if (on == "1") setHorn(true);
  else if (on == "0") setHorn(false);
  else {
    server.send(400, "application/json", "{\"error\":\"on must be 0 or 1\"}");
    return;
  }
  sendStatus();
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(ENA, MOTOR_PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(ENB, MOTOR_PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(BUZZER_PIN, HORN_FREQUENCY, PWM_RESOLUTION);
#else
  ledcSetup(LEFT_PWM_CHANNEL, MOTOR_PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL, MOTOR_PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(BUZZER_PWM_CHANNEL, HORN_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(ENA, LEFT_PWM_CHANNEL);
  ledcAttachPin(ENB, RIGHT_PWM_CHANNEL);
  ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);
#endif
  stopMotors();
  setHorn(false);

  panServo.setPeriodHertz(50);
  tiltServo.setPeriodHertz(50);
  panServo.attach(PAN_SERVO_PIN, 500, 2400);
  tiltServo.attach(TILT_SERVO_PIN, 500, 2400);
  panServo.write(panCurrent);
  tiltServo.write(tiltCurrent);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println();
  Serial.print("Connect to: "); Serial.println(AP_SSID);
  Serial.print("Open: http://"); Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.send_P(200, "text/html", CONTROL_PAGE);
  });
  server.on("/api/status", HTTP_GET, []() { sendStatus(); });
  server.on("/api/drive", HTTP_GET, handleDrive);
  server.on("/api/speed", HTTP_GET, handleSpeed);
  server.on("/api/pan", HTTP_GET, handlePan);
  server.on("/api/tilt", HTTP_GET, handleTilt);
  server.on("/api/horn", HTTP_GET, handleHorn);
  server.onNotFound([]() { server.send(404, "application/json", "{\"error\":\"not found\"}"); });
  server.begin();
}

void loop() {
  server.handleClient();
  updateServoSmoothly(panServo, panCurrent, panTarget, lastPanStep);
  updateServoSmoothly(tiltServo, tiltCurrent, tiltTarget, lastTiltStep);

  if (driveState != STOPPED && millis() - lastMotorCommand > MOTOR_TIMEOUT_MS) {
    stopMotors();
  }
  if (hornIsOn && millis() - lastHornCommand > HORN_TIMEOUT_MS) {
    setHorn(false);
  }
}
