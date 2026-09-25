/*
 * PROJECT : MotorMind Edge AI System
 * VERSION : 8.5 Masterpiece - Majority Voting, Current EMA & Fault Reason
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <math.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// استدعاء ملف الذكاء الاصطناعي
#include "MotorModel.h"

// ==========================================================
// 🛠️ إعدادات الشبكة وتليجرام (عدل بيانات الهوتسبوت هنا)
// ==========================================================
const char* sta_ssid     = "Ahmed"; 
const char* sta_password = "12345678";

const char* ap_ssid      = "MotorMind_AI";
const char* ap_password  = "12345678";

String botToken = "8559134031:AAHOt4RV3OKMfPTKIpW2GA7T3xoTcdcXYcA";
String chatId   = "1106389688";
bool alertSent  = false;

// ==========================================================
// 🛠️ حدود الحماية القطعية (Hard Limits)
// ==========================================================
#define VIB_THRESH   4.50f  
#define AMP_THRESH   5.00f  
#define TEMP_THRESH  55.0f  

#define TEMP_PIN    15
#define ACS_PIN     34
#define MOTOR_RPWM  26
#define MOTOR_LPWM  27
#define FAN_IN1     32
#define FAN_IN2     33
#define BUZZER      5
#define RESET_BTN   4    

#define SD_CS       13
#define SD_SCK      18
#define SD_MISO     19
#define SD_MOSI     23

#define WINDOW_SIZE    64
#define MOTOR_PWM      178
#define PWM_FREQ       5000
#define PWM_RES        8
#define WARMUP_MS      10000

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Adafruit_ADXL345_Unified            accel = Adafruit_ADXL345_Unified(12345);
OneWire                             oneWire(TEMP_PIN);
DallasTemperature                   sensors(&oneWire);
WebServer                           server(80);

float currentTemp    = 0, currentAmp  = 0;
float filteredAmp    = 0; // متغير الفلترة للتيار (EMA)
float rms_val        = 0, kurt_val    = 0;
float crest_val      = 0, p2p_val     = 0;
float peakFreq       = 0, spectralEnergy = 0;
float acsZeroVoltage = 1.65f;
float staticX        = 0, staticY = 0, staticZ = 0;

int    systemStatus     = 0; 
String statusText       = "WARMING UP";
String faultReason      = "NONE"; // حفظ سبب العطل
int    healthScore      = 100;
bool   warmupDone       = false;
bool   systemLocked     = false; 
bool   sdAvailable      = false;
bool   tempError        = false;

// متغيرات الـ Majority Voting (3 من 5)
int faultHistory[5] = {0, 0, 0, 0, 0};
int faultIndex = 0;

int           previousHealth  = 100;
String        healthTrend     = "STABLE";
unsigned long inferenceTimeUs = 0;
int           totalFaults     = 0;
bool          faultCounted    = false;
float         aiProbability   = 0.0f; 

unsigned long motorStartTime  = 0;
unsigned long lastTempRequest = 0;
unsigned long uptimeSeconds   = 0;
unsigned long lastUptimeTick  = 0;
unsigned long lastLogTime     = 0;

SemaphoreHandle_t i2cMutex;

// ==========================================================
// 📡 دالة إرسال التنبيهات على تليجرام
// ==========================================================
void sendTelegramAlert(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); 
    HTTPClient http;
    
    message.replace(" ", "%20");
    message.replace("\n", "%0A");
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + message;
    
    http.begin(client, url);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.print("# Telegram Alert Sent Successfully! Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("# Failed to send Telegram alert. Error Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("# WiFi not connected, cannot send alert.");
  }
}

void logDataToSD() {
  if (!sdAvailable || !warmupDone) return;
  File logFile = SD.open("/log.csv", FILE_APPEND);
  if (logFile) {
    logFile.printf("%lu,%u,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%d,%d,%s\n", 
                   millis(), uptimeSeconds, currentTemp, currentAmp, 
                   rms_val, kurt_val, crest_val, p2p_val, peakFreq, spectralEnergy, systemStatus, healthScore, faultReason.c_str());
    logFile.close();
  }
}

float calculateKurtosis(float* data, int n, float mean, float stdDev) {
  if (stdDev < 0.0001f) return 0.0f;
  float sum4 = 0.0f;
  for (int i = 0; i < n; i++) sum4 += powf((data[i] - mean), 4);
  return (sum4 / (n * powf(stdDev, 4))) - 3.0f;
}

int calcHealthScore() {
  if (systemStatus == 1) return 0;
  int score = 100;
  if (rms_val > 3.8f) score -= (int)(((rms_val - 3.8f) / (VIB_THRESH - 3.8f)) * 50.0f); 
  if (currentAmp > 1.1f) score -= (int)(((currentAmp - 1.1f) / (AMP_THRESH - 1.1f)) * 30.0f);
  if (currentTemp > 40.0f) score -= (int)(((currentTemp - 40.0f) / (TEMP_THRESH - 40.0f)) * 20.0f);
  return constrain(score, 0, 100);
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MotorMind AI Pro v8.5</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:'Segoe UI',sans-serif;background:#0d1117;color:#e6edf3;padding:16px}
  h1{color:#58a6ff;text-align:center;font-size:1.6rem;letter-spacing:3px;margin-bottom:4px}
  .sub{text-align:center;color:#8b949e;font-size:0.8rem;margin-bottom:20px}
  .live{display:inline-block;width:8px;height:8px;background:#3fb950;border-radius:50%;animation:pulse 1.5s infinite;margin-right:6px}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:0.3}}
  .banner{padding:14px 20px;border-radius:10px;text-align:center;font-size:1.3rem;font-weight:700;margin-bottom:20px;transition:all 0.4s;letter-spacing:1px}
  .s0{background:#0d2b1a;border:1px solid #3fb950;color:#3fb950}
  .s1{background:#2b0d0d;border:1px solid #f85149;color:#f85149;animation:blink 0.8s infinite}
  .sw{background:#1a1a2e;border:1px solid #58a6ff;color:#58a6ff}
  @keyframes blink{50%{opacity:0.5}}
  .warmup-bar{height:6px;background:#21262d;border-radius:3px;margin-bottom:20px;overflow:hidden}
  .warmup-fill{height:100%;background:#58a6ff;border-radius:3px;transition:width 0.5s}
  .health-wrap{display:flex;justify-content:center;margin-bottom:20px}
  .health-ring{position:relative;width:120px;height:120px}
  .health-ring svg{transform:rotate(-90deg)}
  .ring-bg{fill:none;stroke:#21262d;stroke-width:10}
  .ring-fill{fill:none;stroke-width:10;stroke-linecap:round;transition:stroke-dashoffset 0.8s}
  .ring-text{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);text-align:center;font-size:1.8rem;font-weight:700}
  .ring-label{font-size:0.65rem;color:#8b949e;display:block}
  .trend-badge{text-align:center;font-size:0.85rem;font-weight:bold;color:#8b949e;margin-top:-10px;margin-bottom:20px;}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:12px;margin-bottom:20px}
  .card{background:#161b22;padding:16px;border-radius:10px;border-left:3px solid #30363d}
  .card h3{font-size:0.7rem;color:#8b949e;text-transform:uppercase;letter-spacing:1px;margin-bottom:8px}
  .val{font-size:1.8rem;font-weight:700}
  .unit{font-size:0.75rem;color:#8b949e;margin-left:2px}
  .c-vib{border-color:#58a6ff}.c-kurt{border-color:#a371f7}
  .c-crest{border-color:#3fb950}.c-p2p{border-color:#d2a679}
  .c-temp{border-color:#f85149}.c-amp{border-color:#d29922}
  .chart-section{background:#161b22;border-radius:10px;padding:16px;margin-bottom:16px}
  .chart-title{font-size:0.75rem;color:#8b949e;text-transform:uppercase;letter-spacing:1px;margin-bottom:8px}
  canvas{width:100%!important;height:80px!important}
  .info-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px}
  .info-box{background:#161b22;border-radius:10px;padding:14px}
  .info-box h3{font-size:0.7rem;color:#8b949e;text-transform:uppercase;margin-bottom:8px}
  .info-row{display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid #21262d;font-size:0.8rem}
  .info-row:last-child{border:none}
  .info-val{color:#58a6ff;font-weight:600}
  .btn-row{display:flex;gap:10px;justify-content:center;margin-top:16px;flex-wrap:wrap;}
  .btn{padding:10px 24px;font-size:0.9rem;border:none;border-radius:6px;cursor:pointer;font-weight:700;transition:opacity 0.2s}
  .btn:hover{opacity:0.8}
  .btn-web{background:#3fb950;color:#0d1117}
  .btn-sd{background:#a371f7;color:#fff}
  .btn-clear{background:#21262d;color:#e6edf3;border:1px solid #30363d}
  .btn-reset{background:#f85149;color:#fff;border:1px solid #f85149}
  .footer{text-align:center;color:#8b949e;font-size:0.75rem;margin-top:16px}
</style>
</head>
<body>
<h1><span class="live"></span>MotorMind AI Pro</h1>
<p class="sub">V8.5 Masterpiece · Voting, EMA & Telemetry</p>
<div id="banner" class="banner sw">⏳ WARMING UP...</div>
<div class="warmup-bar"><div id="warmupFill" class="warmup-fill" style="width:0%"></div></div>
<div class="health-wrap">
  <div class="health-ring">
    <svg viewBox="0 0 120 120" width="120" height="120">
      <circle class="ring-bg" cx="60" cy="60" r="50"/>
      <circle id="ringFill" class="ring-fill" cx="60" cy="60" r="50"
              stroke="#3fb950" stroke-dasharray="314" stroke-dashoffset="0"/>
    </svg>
    <div class="ring-text">
      <span id="healthNum">100</span>
      <span class="ring-label">HEALTH</span>
    </div>
  </div>
</div>
<div id="trendLabel" class="trend-badge">Trend: STABLE</div>
<div class="grid">
  <div class="card c-vib"><h3>Vibration RMS</h3>
    <div class="val"><span id="rms">0.00</span><span class="unit">g</span></div></div>
  <div class="card c-kurt"><h3>Kurtosis</h3>
    <div class="val"><span id="kurt">0.00</span></div></div>
  <div class="card c-crest"><h3>Crest Factor</h3>
    <div class="val"><span id="crest">0.00</span></div></div>
  <div class="card c-p2p"><h3>Peak-to-Peak</h3>
    <div class="val"><span id="p2p">0.00</span><span class="unit">g</span></div></div>
  <div class="card c-temp"><h3>Temperature</h3>
    <div class="val"><span id="temp">0.00</span><span class="unit">°C</span></div></div>
  <div class="card c-amp"><h3>Current</h3>
    <div class="val"><span id="amp">0.00</span><span class="unit">A</span></div></div>
</div>
<div class="chart-section">
  <div class="chart-title">▸ Vibration RMS — Last 40 Windows</div>
  <canvas id="rmsChart"></canvas>
</div>
<div class="chart-section">
  <div class="chart-title">▸ Current — Last 40 Windows</div>
  <canvas id="ampChart"></canvas>
</div>
<div class="info-grid">
  <div class="info-box">
    <h3>AI Inference Details</h3>
    <div class="info-row"><span>Inference Time</span><span id="inferTime" class="info-val">0 µs</span></div>
    <div class="info-row"><span>Fault Probability</span><span id="probLabel" class="info-val">0.0%</span></div>
    <div class="info-row"><span>Fault Reason</span><span id="reasonLabel" class="info-val">—</span></div>
    <div class="info-row"><span>Total Faults</span><span id="faultsCount" class="info-val">0</span></div>
  </div>
  <div class="info-box">
    <h3>Data &amp; Storage</h3>
    <div class="info-row"><span>Log Rate</span><span class="info-val">1 Hz</span></div>
    <div class="info-row"><span>Storage Media</span><span id="sdStatus" class="info-val">OFFLINE</span></div>
    <div class="info-row"><span>Uptime</span><span id="uptime" class="info-val">0s</span></div>
  </div>
</div>
<div class="btn-row">
  <button class="btn btn-web" onclick="downloadWebLog()">📱 Download Web Log</button>
  <button class="btn btn-sd" onclick="downloadSDLog()">💾 Download SD Log</button>
  <button class="btn btn-clear"  onclick="clearLog()">🗑 Clear Logs</button>
  <button class="btn btn-reset" onclick="fetch('/reset').then(()=>location.reload())">🔄 Reset System</button>
</div>
<div class="footer" id="footer">Waiting for data...</div>
<script>
function makeChart(id,color){
  const c=document.getElementById(id); const ctx=c.getContext('2d'); const data=new Array(40).fill(0);
  c.width=c.offsetWidth;c.height=80;
  function draw(){
    const W=c.width,H=c.height; ctx.clearRect(0,0,W,H); ctx.strokeStyle='#21262d';ctx.lineWidth=0.5;
    for(let i=1;i<4;i++){ctx.beginPath();ctx.moveTo(0,H/4*i);ctx.lineTo(W,H/4*i);ctx.stroke();}
    const mx=Math.max(...data,0.1); ctx.strokeStyle=color;ctx.lineWidth=1.5; ctx.shadowColor=color;ctx.shadowBlur=4; ctx.beginPath();
    data.forEach((v,i)=>{
      const x=(i/(data.length-1))*W; const y=H-(v/mx)*(H*0.85)-H*0.05; i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
    });
    ctx.stroke();ctx.shadowBlur=0; ctx.fillStyle=color+'22'; ctx.lineTo(W,H);ctx.lineTo(0,H);ctx.closePath();ctx.fill();
  }
  return{push(v){data.push(v);if(data.length>40)data.shift();draw();}};
}
const rmsChart=makeChart('rmsChart','#58a6ff');
const ampChart=makeChart('ampChart','#d29922');

let logData=["Timestamp,Temp_C,Current_A,RMS,Kurtosis,CrestFactor,PeakToPeak,PeakFreq,Energy,Status,Reason"];
const banner=document.getElementById('banner');

function updateRing(score){
  const circ=2*Math.PI*50; const offset=circ*(1-score/100); const fill=document.getElementById('ringFill');
  fill.style.strokeDashoffset=offset; fill.style.stroke=score>50?'#3fb950':'#f85149';
  document.getElementById('healthNum').innerText=score;
  document.getElementById('healthNum').style.color=fill.style.stroke;
}

const startTime=Date.now();
const WARMUP_MS=10000;

setInterval(()=>{
  const elapsed=Date.now()-startTime;
  const pct=Math.min(100,(elapsed/WARMUP_MS)*100);
  document.getElementById('warmupFill').style.width=pct+'%';

  fetch('/data').then(r=>r.json()).then(d=>{
    document.getElementById('rms').innerText   =d.rms.toFixed(2);
    document.getElementById('kurt').innerText  =d.kurt.toFixed(2);
    document.getElementById('crest').innerText =d.crest.toFixed(2);
    document.getElementById('p2p').innerText   =d.p2p.toFixed(2);
    document.getElementById('temp').innerText  =d.temp.toFixed(2);
    document.getElementById('amp').innerText   =d.amp.toFixed(2);
    document.getElementById('uptime').innerText=d.uptime+'s';
    
    document.getElementById('inferTime').innerText = d.infer + ' µs';
    document.getElementById('probLabel').innerText = (d.prob * 100).toFixed(1) + '%';
    document.getElementById('faultsCount').innerText = d.faults;
    document.getElementById('trendLabel').innerText = 'Trend: ' + d.trend;
    
    document.getElementById('sdStatus').innerText = d.sd ? "SD ACTIVE" : "OFFLINE";
    document.getElementById('sdStatus').style.color = d.sd ? "#3fb950" : "#8b949e";

    if(d.warmup){
      banner.innerText='⏳ WARMING UP... ('+d.warmupLeft+'s remaining)';
      banner.className='banner sw';
      document.getElementById('reasonLabel').innerText='—';
    } else {
      if(d.statusCode == 0){
        banner.innerText="✅ SYSTEM HEALTHY";
        banner.className='banner s0';
        document.getElementById('reasonLabel').innerText="NONE";
      } else {
        banner.innerText="🚨 FAULT: " + d.reason;
        banner.className='banner s1';
        document.getElementById('reasonLabel').innerText=d.reason;
      }
    }

    updateRing(d.health);
    rmsChart.push(d.rms);
    ampChart.push(d.amp);

    const t=new Date().toISOString().substr(11,8);
    logData.push(`${t},${d.temp.toFixed(2)},${d.amp.toFixed(2)},${d.rms.toFixed(2)},${d.kurt.toFixed(2)},${d.crest.toFixed(2)},${d.p2p.toFixed(2)},${d.freq.toFixed(1)},${d.energy.toFixed(3)},${d.statusCode},${d.reason}`);
    document.getElementById('footer').innerText=`Last update: ${t}  |  Web Log: ${logData.length-1} rows`;
  }).catch(()=>{});
},500);

function getTimestamp() {
  const d = new Date();
  const yyyy = d.getFullYear();
  const mm = String(d.getMonth() + 1).padStart(2, '0');
  const dd = String(d.getDate()).padStart(2, '0');
  const hh = String(d.getHours()).padStart(2, '0');
  const min = String(d.getMinutes()).padStart(2, '0');
  return yyyy + "-" + mm + "-" + dd + "_" + hh + "-" + min;
}

function downloadWebLog(){
  if(logData.length <= 1) { alert("No data available to download yet!"); return; }
  const blob = new Blob([logData.join("\n")], { type: 'text/csv;charset=utf-8;' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.setAttribute("href", url);
  link.setAttribute("download", "MotorMind_WebLog_" + getTimestamp() + ".csv");
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

function downloadSDLog() {
  window.open('/download?ts=' + getTimestamp());
}

function clearLog(){ logData=["Timestamp,Temp_C,Current_A,RMS,Kurtosis,CrestFactor,PeakToPeak,PeakFreq,Energy,Status,Reason"]; }
</script>
</body>
</html>
)rawliteral";

void handleRoot(){server.send(200,"text/html",INDEX_HTML);}

void handleData(){
  bool inWarmup=!warmupDone;
  int warmupLeft=inWarmup?(int)((WARMUP_MS-(millis()-motorStartTime))/1000):0;
  if(warmupLeft<0)warmupLeft=0;
  String json="{";
  json+="\"temp\":"      +String(currentTemp, 2)+",";
  json+="\"amp\":"       +String(currentAmp,  2)+",";
  json+="\"rms\":"       +String(rms_val,      4)+",";
  json+="\"kurt\":"      +String(kurt_val,     4)+",";
  json+="\"crest\":"     +String(crest_val,    4)+",";
  json+="\"p2p\":"       +String(p2p_val,      4)+",";
  json+="\"freq\":"      +String(peakFreq,     2)+",";
  json+="\"energy\":"    +String(spectralEnergy,4)+",";
  json+="\"statusCode\":"+String(systemStatus)   +",";
  json+="\"health\":"    +String(healthScore)    +",";
  json+="\"uptime\":"    +String(uptimeSeconds)  +",";
  json+="\"warmup\":"    +(inWarmup?String("true"):String("false"))+",";
  json+="\"warmupLeft\":"+String(warmupLeft)     +",";
  json+="\"sd\":"        +(sdAvailable?String("true"):String("false"))+",";
  json+="\"status\":\""  +statusText             +"\",";
  json+="\"reason\":\""  +faultReason            +"\","; // إضافة السبب للداشبورد
  json+="\"infer\":"     +String(inferenceTimeUs)+",";
  json+="\"prob\":"      +String(aiProbability, 4)+","; 
  json+="\"faults\":"    +String(totalFaults)    +",";
  json+="\"trend\":\""   +healthTrend            +"\"";
  json+="}";
  server.send(200,"application/json",json);
}

void handleDownload() {
  if (!sdAvailable) { server.send(404, "text/plain", "SD Card Not Found"); return; }
  File downloadFile = SD.open("/log.csv", FILE_READ);
  if (downloadFile) {
    String fileName = "MotorMind_SDLog";
    if(server.hasArg("ts")) {
        fileName += "_" + server.arg("ts");
    }
    fileName += ".csv";
    
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
    server.streamFile(downloadFile, "text/csv");
    downloadFile.close();
  } else {
    server.send(500, "text/plain", "Failed to open log file");
  }
}

// ==========================================================
// 🌐 إعدادات الشبكة المزدوجة (AP + STA)
// ==========================================================
void Task_IoT(void* pvParameters){
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("# AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(sta_ssid, sta_password);
  Serial.print("# Connecting to Internet");
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
    retries++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
      Serial.println("\n# Internet Connected! Telegram Alerts ACTIVE.");
  } else {
      Serial.println("\n# Failed to connect to Internet. Telegram Alerts Offline.");
  }

  server.on("/",    HTTP_GET,handleRoot);
  server.on("/data",HTTP_GET,handleData);
  server.on("/download", HTTP_GET, handleDownload);
  
  server.on("/reset", HTTP_GET, [](){
    systemLocked     = false;
    systemStatus     = 0;
    faultReason      = "NONE";
    for(int i=0; i<5; i++) faultHistory[i] = 0; // تصفير الـ Voting
    statusText       = "HEALTHY / RUN";
    faultCounted     = false;
    aiProbability    = 0.0f;
    alertSent        = false; 
    
    sendTelegramAlert("MOTOR-MIND: System reset completed via Web Dashboard. Status: HEALTHY");
    server.send(200, "text/plain", "System Reset OK");
  });

  server.begin();
  
  for(;;){
      server.handleClient();
      
      static unsigned long lastWifiCheck = 0;
      if (millis() - lastWifiCheck > 5000) {
          if (WiFi.status() != WL_CONNECTED) {
             WiFi.reconnect(); 
          }
          lastWifiCheck = millis();
      }

      vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}

void setup(){
  Serial.begin(115200);

  pinMode(RESET_BTN, INPUT_PULLUP); 

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if(!SD.begin(SD_CS)) {
    Serial.println("# SD Card Not Found - Web Download Only");
    sdAvailable = false;
  } else {
    Serial.println("# SD Card Ready.");
    sdAvailable = true;
    if(!SD.exists("/log.csv")) {
      File f = SD.open("/log.csv", FILE_WRITE);
      // إضافة عمود السبب لملف الـ SD
      if(f) { f.println("Timestamp,Uptime_S,Temp_C,Current_A,RMS,Kurtosis,CrestFactor,P2P,PeakFreq,Energy,Status,HealthIndex,Reason"); f.close(); }
    }
  }

  Wire.begin(21,22);
  Wire.setClock(400000); 
  Wire.setTimeout(100); 

  pinMode(FAN_IN1,   OUTPUT);digitalWrite(FAN_IN1,   LOW);
  pinMode(FAN_IN2,   OUTPUT);digitalWrite(FAN_IN2,   LOW);
  pinMode(BUZZER,    OUTPUT);digitalWrite(BUZZER,    LOW);
  pinMode(MOTOR_LPWM,OUTPUT);digitalWrite(MOTOR_LPWM,LOW);
  ledcAttach(MOTOR_RPWM,PWM_FREQ,PWM_RES);

  u8g2.begin();
  i2cMutex = xSemaphoreCreateMutex();

  if(!accel.begin(0x53)){
    Serial.println("ERROR: ADXL345 not found!");
  } else {
    accel.setRange(ADXL345_RANGE_16_G);
    accel.setDataRate(ADXL345_DATARATE_200_HZ);
  }
  
  sensors.begin();
  sensors.setWaitForConversion(false);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0,14,"MotorMind V8.5");
  u8g2.drawStr(0,28,"Calibrating...");
  u8g2.sendBuffer();

  long acsSum=0;
  for(int i=0;i<200;i++){
      acsSum+=analogRead(ACS_PIN);
      vTaskDelay(pdMS_TO_TICKS(5)); 
  }
  acsZeroVoltage=((acsSum/200.0f)*3.3f)/4095.0f;
  
  float sx=0,sy=0,sz=0;
  for(int i=0;i<200;i++){
    sensors_event_t ev;
    accel.getEvent(&ev);
    sx+=ev.acceleration.x;sy+=ev.acceleration.y;sz+=ev.acceleration.z;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  staticX=sx/200.0f;staticY=sy/200.0f;staticZ=sz/200.0f;

  for(int s=0;s<=MOTOR_PWM;s++){ ledcWrite(MOTOR_RPWM,s); vTaskDelay(pdMS_TO_TICKS(100)); }
  motorStartTime = millis();

  xTaskCreatePinnedToCore(Task_IoT,"IoT",12288,NULL,1,NULL,1);
  Serial.println("# MotorMind v8.5 Ready - Entering Loop");
}

void loop(){
  if (digitalRead(RESET_BTN) == LOW) {
      systemLocked = false;
      systemStatus = 0;
      faultReason = "NONE";
      for(int i=0; i<5; i++) faultHistory[i] = 0; // تصفير الـ Voting
      faultCounted = false;
      aiProbability = 0.0f;
      alertSent = false; 
      statusText = "HEALTHY / RUN";
      noTone(BUZZER);
      
      sendTelegramAlert("MOTOR-MIND: System reset completed successfully. Status: HEALTHY");
      vTaskDelay(pdMS_TO_TICKS(300));
  }

  if(millis()-lastUptimeTick>=1000){uptimeSeconds++;lastUptimeTick=millis();}

  tempError = false;
  if(millis()-lastTempRequest>=800){
    float tempRead = sensors.getTempCByIndex(0);
    if(tempRead == -127.00f || tempRead == 85.00f) {
        tempError = true;
    } else { currentTemp = tempRead; }
    sensors.requestTemperatures();
    lastTempRequest=millis();
  }

  if(!warmupDone) {
      if(millis() - motorStartTime >= WARMUP_MS) { warmupDone = true; } 
      else {
          if ((millis() / 500) % 2 == 0) {
              xSemaphoreTake(i2cMutex, portMAX_DELAY);
              u8g2.clearBuffer(); u8g2.drawStr(0,28,"Warming up AI..."); u8g2.sendBuffer();
              xSemaphoreGive(i2cMutex);
          }
          vTaskDelay(pdMS_TO_TICKS(100)); return;
      }
  }

  long acsSum=0;
  for(int i=0;i<32;i++){acsSum+=analogRead(ACS_PIN);delayMicroseconds(100);}
  float voltage=((acsSum/32.0f)*3.3f)/4095.0f;
  
  float rawAmp=abs((voltage-acsZeroVoltage)/0.185f);
  if(rawAmp<0.06f) rawAmp=0.0f;
  
  // 🚀 تطبيق الـ EMA Filter
  filteredAmp = (0.8f * filteredAmp) + (0.2f * rawAmp);
  currentAmp = filteredAmp;

  float magnitudes[WINDOW_SIZE];
  float sq_sum=0,sum_mag=0;
  float max_mag=-1,min_mag=9999;
  unsigned long nextSample=micros();
  bool adxlError = false;

  for(int i=0;i<WINDOW_SIZE;i++){
    while(micros() - nextSample < 5000UL){ yield(); } 
    nextSample += 5000UL;

    xSemaphoreTake(i2cMutex,portMAX_DELAY);
    sensors_event_t ev; accel.getEvent(&ev);
    xSemaphoreGive(i2cMutex);

    if(ev.acceleration.x == 0.00f && ev.acceleration.y == 0.00f) adxlError = true;

    float dx=ev.acceleration.x-staticX; float dy=ev.acceleration.y-staticY; float dz=ev.acceleration.z-staticZ;
    float mag=sqrtf(dx*dx+dy*dy+dz*dz);

    magnitudes[i]=mag; sum_mag+=mag; sq_sum+=mag*mag;
    if(mag>max_mag)max_mag=mag;
    if(mag<min_mag)min_mag=mag;
  }

  float mean    =sum_mag/WINDOW_SIZE;
  rms_val       =sqrtf(sq_sum/WINDOW_SIZE);
  float variance=max(0.0f,(sq_sum/WINDOW_SIZE)-(mean*mean));
  float stdDev  =sqrtf(variance);
  kurt_val      =calculateKurtosis(magnitudes,WINDOW_SIZE,mean,stdDev);
  p2p_val       =max_mag-min_mag;
  crest_val     =(rms_val>0.001f)?(max_mag/rms_val):0.0f;
  
  spectralEnergy = sq_sum / WINDOW_SIZE;
  int zeroCrossings = 0;
  for(int i=1; i<WINDOW_SIZE; i++){
      if ((magnitudes[i] - mean) * (magnitudes[i-1] - mean) < 0) zeroCrossings++;
  }
  peakFreq = (zeroCrossings / 2.0f) * (200.0f / WINDOW_SIZE);

  if (!systemLocked) {
      int rawState = 0;
      String currentPendingReason = "NONE";
      
      if (tempError) { rawState = 1; aiProbability = 1.0f; currentPendingReason = "TEMP_SENSOR_ERR"; }
      else if (adxlError) { rawState = 1; aiProbability = 1.0f; currentPendingReason = "VIB_SENSOR_ERR"; }
      else if (currentAmp >= AMP_THRESH) { rawState = 1; aiProbability = 1.0f; currentPendingReason = "OVERCURRENT"; }
      else if (rms_val < 1.0f && currentAmp == 0.0f) { rawState = 0; aiProbability = 0.0f; } 
      else {
          double modelInput[8] = {
              (double)rms_val, (double)kurt_val, (double)crest_val, (double)p2p_val, 
              (double)currentAmp, (double)peakFreq, (double)spectralEnergy, (double)currentTemp 
          };
          
          unsigned long t0 = micros(); 
          double raw_score = score(modelInput);
          inferenceTimeUs = micros() - t0;
          
          aiProbability = 1.0 / (1.0 + exp(-raw_score)); 
          if (aiProbability >= 0.5f) { 
              rawState = 1; 
              currentPendingReason = "AI_PREDICTION"; 
          }
      }

      // 🚀 تطبيق الـ Majority Voting (3 من 5)
      faultHistory[faultIndex] = rawState;
      faultIndex = (faultIndex + 1) % 5;
      
      int sumFaults = 0;
      for(int i=0; i<5; i++) sumFaults += faultHistory[i];

      if (sumFaults >= 3) {
          systemStatus = 1;
          systemLocked = true; 
          faultReason = currentPendingReason;
          statusText = "LOCKED: " + faultReason;
          Serial.println("!!! SYSTEM FAULT LATCHED: " + faultReason + " !!!");
      } else {
          statusText = "HEALTHY / RUN";
      }
  }

  previousHealth = healthScore;
  healthScore = calcHealthScore();
  
  if (healthScore < previousHealth - 2) healthTrend = "DECLINING";
  else if (healthScore > previousHealth + 2) healthTrend = "IMPROVING";
  else healthTrend = "STABLE";

  if (systemStatus == 1) {
      if (!faultCounted) {
          totalFaults++;
          faultCounted = true;
          
          if (!alertSent) {
             String alertMsg = "🚨 MOTOR-MIND ALERT: FAULT DETECTED!\n\n";
             alertMsg += "Reason: " + faultReason + "\n"; // إضافة السبب لرسالة التليجرام
             alertMsg += "Vib RMS: " + String(rms_val, 2) + " g\n";
             alertMsg += "Current: " + String(currentAmp, 2) + " A\n";
             alertMsg += "Temp: " + String(currentTemp, 1) + " C\n";
             alertMsg += "AI Prob: " + String(aiProbability * 100, 1) + "%\n";
             alertMsg += "Health: " + String(healthScore) + "%\n\n";
             alertMsg += "System Locked. Needs Manual Reset.";
             
             sendTelegramAlert(alertMsg);
             alertSent = true;
          }
      }
  } else {
      faultCounted = false;
      alertSent = false;
  }

  if (systemStatus == 0) { 
    digitalWrite(FAN_IN1, LOW); digitalWrite(FAN_IN2, LOW);
    noTone(BUZZER);
    ledcWrite(MOTOR_RPWM, MOTOR_PWM);
  } 
  else if (systemStatus == 1) { 
    digitalWrite(FAN_IN1, HIGH); digitalWrite(FAN_IN2, LOW); 
    ledcWrite(MOTOR_RPWM, 0); 
    if ((millis() / 200) % 2 == 0) tone(BUZZER, 2000); else noTone(BUZZER); 
  }

  if (millis() - lastLogTime >= 1000) {
      logDataToSD();
      lastLogTime = millis();
  }

  xSemaphoreTake(i2cMutex,portMAX_DELAY);
  u8g2.clearBuffer(); u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0,10,"MotorMind V8.5");
  u8g2.setCursor(0,24);u8g2.print(statusText);
  u8g2.setCursor(0,38);u8g2.print("H:"); u8g2.print(healthScore); u8g2.print("% P:"); u8g2.print((int)(aiProbability*100)); u8g2.print("%");
  u8g2.setCursor(0,52);u8g2.print("Amp:");u8g2.print(currentAmp,1); u8g2.print(" T:"); u8g2.print(inferenceTimeUs); u8g2.print("us");
  u8g2.sendBuffer();
  xSemaphoreGive(i2cMutex);
  
  vTaskDelay(pdMS_TO_TICKS(10)); 
}
