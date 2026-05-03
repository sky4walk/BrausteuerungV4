/*
 * ╔══════════════════════════════════════════════════════════╗
 * ║           BRAUSTEUERUNG — Wemos D1 Mini                 ║
 * ║  DS18B20 → D2 | Buzzer → D3 | RCSwitch → D8            ║
 * ╚══════════════════════════════════════════════════════════╝
 *
 * Board-Paket:  esp8266 by ESP8266 Community  v3.1.2
 * Board:        LOLIN(WEMOS) D1 R2 & mini
 * Flash Size:   4MB (FS: 2MB, OTA: ~1MB)
 * Upload Speed: 921600
 * SSL:          Basic SSL ciphers
 *
 * Bibliotheken (Arduino IDE → Bibliotheken verwalten):
 *   Core (Teil des ESP8266-Pakets v3.1.2):
 *   - ESP8266WiFi
 *   - ESP8266WebServer
 *   - ESP8266mDNS
 *   - LittleFS
 *   - ESP8266HTTPUpdateServer
 *
 *   Extern (manuell installieren):
 *   - OneWire           v2.3.7  (Paul Stoffregen)
 *   - DallasTemperature v3.9.0  (Miles Burton)
 *   - PID               v1.2.0  (Brett Beauregard)
 *   - RCSwitch          v2.6.4  (sui77)
 *   - ArduinoJson       v6.21.5 (Benoit Blanchon) ← unbedingt v6.x!
 *
 * Web-Interface:
 *   - index.html  → PROGMEM (eingebettet) oder LittleFS (/index.html)
 *   - /tools      → Upload von index.html und Firmware OTA
 *   - /update     → ESP8266HTTPUpdateServer OTA (admin/brau2024)
 *
 * LittleFS Dateien:
 *   - /config.json  → RCSwitch Codes, PID-Parameter
 *   - /wlan.json    → WLAN Credentials
 *   - /rezept.bml   → aktuell geladenes Braurezept
 *   - /index.html   → optionale Web-UI (überschreibt PROGMEM)
 *
 * Reset:
 *   - Flash-Knopf (D3) beim Start gedrückt halten → löscht WLAN-Config
 *   - ESP startet dann im AP-Modus: SSID "Brausteuerung" (offen)
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PID_v1.h>
#include <RCSwitch.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPUpdateServer.h>  // OTA Firmware Update

// ── PINS ────────────────────────────────────────────────────
#define PIN_DS18B20   4   // D2
#define PIN_BUZZER    0   // D3
#define PIN_RCSWITCH  15  // D8

// ── WLAN ────────────────────────────────────────────────────
// Credentials werden aus wlan.json geladen (LittleFS)
// Hardcoded nur Fallback-Hostname
const char* HOSTNAME = "brausteuerung";

// WLAN-Modus
bool apModus = false;  // true = Access Point für WLAN-Einrichtung

// AP HTML (minimal, kein PROGMEM nötig da nur temporär)
const char AP_HTML[] PROGMEM =
  "<!DOCTYPE html><html lang=\"de\"><head>"
  "<meta charset=\"UTF-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Brausteuerung WLAN</title>"
  "<style>"
  "body{font-family:monospace;background:#0F1410;color:#C8DCC8;padding:24px;max-width:400px;margin:0 auto}"
  "h2{color:#7DDB9A;font-size:1.4rem;margin-bottom:20px}"
  "label{font-size:0.75rem;color:#5A7A5E;text-transform:uppercase;letter-spacing:0.1em;display:block;margin-bottom:4px}"
  "input,select{width:100%;background:#1C2620;border:1px solid #2A3D2E;color:#E8F4E8;padding:10px;margin-bottom:16px;border-radius:2px;font-family:monospace;font-size:0.9rem}"
  "button{background:#4CAF6E;color:#0F1410;border:none;padding:12px 24px;width:100%;font-weight:bold;font-size:0.9rem;cursor:pointer;border-radius:2px;letter-spacing:0.1em}"
  "button:hover{background:#7DDB9A}"
  ".hint{font-size:0.72rem;color:#5A7A5E;margin-bottom:20px;line-height:1.6}"
  ".net{padding:8px;border:1px solid #2A3D2E;margin-bottom:6px;cursor:pointer;border-radius:2px}"
  ".net:hover{border-color:#4CAF6E;color:#7DDB9A}"
  ".rssi{float:right;color:#5A7A5E}"
  "#status{margin-top:16px;color:#D4882A;font-size:0.8rem;min-height:20px}"
  "</style></head><body>"
  "<h2>🍺 WLAN Einrichtung</h2>"
  "<p class=\"hint\">Wähle dein WLAN-Netz oder gib die SSID manuell ein.<br>Erreichbar unter: <b style=\"color:#7DDB9A\">http://brausteuerung.local</b></p>"
  "<div id=\"nets\"><p style=\"color:#5A7A5E\">Suche Netzwerke...</p></div>"
  "<br>"
  "<label>SSID</label>"
  "<input type=\"text\" id=\"ssid\" placeholder=\"WLAN-Name\">"
  "<label>Passwort</label>"
  "<input type=\"password\" id=\"pw\" placeholder=\"Passwort\">"
  "<button onclick=\"speichern()\">Verbinden &amp; Speichern</button>"
  "<div id=\"status\"></div>"
  "<script>"
  "fetch('/ap/scan').then(r=>r.json()).then(d=>{"
  "  const div=document.getElementById('nets');"
  "  if(!d.length){div.innerHTML='<p style=\"color:#5A7A5E\">Keine Netzwerke gefunden</p>';return;}"
  "  div.innerHTML=d.map(n=>"
  "    '<div class=\"net\" onclick=\"document.getElementById(\\x27ssid\\x27).value=\\x27'+n.ssid+'\\x27\">'"
  "    +n.ssid+'<span class=\"rssi\">'+n.rssi+' dBm</span></div>'"
  "  ).join('');"
  "});"
  "function speichern(){"
  "  const ssid=document.getElementById('ssid').value.trim();"
  "  const pw=document.getElementById('pw').value;"
  "  if(!ssid){document.getElementById('status').textContent='Bitte SSID eingeben';return;}"
  "  document.getElementById('status').textContent='Speichere...';"
  "  fetch('/ap/save',{method:'POST',headers:{'Content-Type':'application/json'},"
  "    body:JSON.stringify({ssid,pw})})"
  "  .then(r=>r.json()).then(d=>{"
  "    document.getElementById('status').textContent='✓ Gespeichert — Neustart...';"
  "    setTimeout(()=>location.href='http://brausteuerung.local',4000);"
  "  }).catch(()=>document.getElementById('status').textContent='Fehler');"
  "}"
  "</script></body></html>";

// ── SETUP SEITE (PROGMEM) ────────────────────────────────────
// Wird angezeigt wenn keine index.html in LittleFS vorhanden ist
const char SETUP_HTML[] PROGMEM =
  "<!DOCTYPE html><html lang=\"de\"><head>"
  "<meta charset=\"UTF-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Brausteuerung Setup</title>"
  "<style>"
  "body{font-family:monospace;background:#0F1410;color:#C8DCC8;padding:32px 24px;max-width:420px;margin:0 auto;text-align:center}"
  "h1{color:#7DDB9A;font-size:1.6rem;margin-bottom:8px;letter-spacing:0.08em}"
  ".sub{color:#5A7A5E;font-size:0.75rem;margin-bottom:40px;letter-spacing:0.05em}"
  ".box{background:#161D17;border:1px solid #2A3D2E;border-radius:3px;padding:28px 24px;margin-bottom:16px}"
  "h2{color:#4CAF6E;font-size:0.85rem;text-transform:uppercase;letter-spacing:0.12em;margin-bottom:12px}"
  "p{font-size:0.78rem;color:#5A7A5E;line-height:1.7;margin-bottom:16px}"
  "b{color:#D4882A}"
  "input[type=file]{width:100%;background:#1C2620;border:1px solid #2A3D2E;color:#C8DCC8;"
  "padding:10px;border-radius:2px;font-family:monospace;font-size:0.78rem;margin-bottom:12px}"
  ".btn{width:100%;background:#4CAF6E;color:#0F1410;border:none;padding:13px;"
  "font-family:monospace;font-size:0.85rem;font-weight:bold;border-radius:2px;"
  "cursor:pointer;letter-spacing:0.08em}"
  ".btn:hover{background:#7DDB9A}"
  ".status{margin-top:12px;font-size:0.78rem;min-height:20px;color:#5A7A5E}"
  ".status.ok{color:#7DDB9A}"
  ".status.err{color:#C0504A}"
  "</style></head><body>"
  "<h1>&#x1F37A; BRAUSTEUERUNG</h1>"
  "<div class=\"sub\">Ersteinrichtung</div>"
  "<div class=\"box\">"
  "<h2>&#x2B06; index.html hochladen</h2>"
  "<p>Die Web-Oberfl&auml;che muss einmalig hochgeladen werden.<br>"
  "W&auml;hle die <b>index.html</b> Datei und klicke auf Hochladen.</p>"
  "<input type=\"file\" id=\"f\" accept=\".html\">"
  "<button class=\"btn\" onclick=\"upload()\">&#x2B06; Hochladen &amp; Starten</button>"
  "<div class=\"status\" id=\"st\"></div>"
  "</div>"
  "<script>"
  "async function upload(){"
  "const file=document.getElementById('f').files[0];"
  "if(!file){document.getElementById('st').textContent='Bitte Datei w\u00e4hlen';return;}"
  "const st=document.getElementById('st');"
  "st.className='status';st.textContent='\u23f3 Lade hoch...';"
  "const form=new FormData();form.append('file',file);"
  "try{"
  "const r=await fetch('/upload/html',{method:'POST',body:form});"
  "const j=await r.json();"
  "if(j.ok){st.className='status ok';"
  "st.textContent='\u2713 Erfolgreich! Seite wird neu geladen...';"
  "setTimeout(()=>location.reload(),1500);}"
  "else{st.className='status err';st.textContent='\u2717 '+(j.error||'Fehler');}"
  "}catch(e){st.className='status err';st.textContent='\u2717 Verbindungsfehler';}"
  "}"
  "</script></body></html>";


// ── TOOLS SEITE (PROGMEM) ────────────────────────────────────
// Erreichbar unter /tools — HTML + Firmware Upload ohne Haupt-UI
const char TOOLS_HTML[] PROGMEM =
  "<!DOCTYPE html><html lang=\"de\"><head>"
  "<meta charset=\"UTF-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Brausteuerung Tools</title>"
  "<style>"
  "body{font-family:'IBM Plex Mono',monospace;background:#0F1410;color:#C8DCC8;padding:24px;max-width:500px;margin:0 auto}"
  "h1{color:#7DDB9A;font-size:1.4rem;margin-bottom:4px;letter-spacing:0.08em}"
  ".sub{color:#5A7A5E;font-size:0.72rem;margin-bottom:32px;letter-spacing:0.05em}"
  "h2{color:#4CAF6E;font-size:0.85rem;text-transform:uppercase;letter-spacing:0.12em;"
  "border-bottom:1px solid #2A3D2E;padding-bottom:8px;margin:24px 0 12px}"
  "p{font-size:0.78rem;color:#5A7A5E;line-height:1.7;margin-bottom:12px}"
  "b{color:#D4882A}"
  ".row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-bottom:8px}"
  "input[type=file]{background:#1C2620;border:1px solid #2A3D2E;color:#C8DCC8;"
  "padding:8px;border-radius:2px;font-family:monospace;font-size:0.78rem;flex:1}"
  ".btn{font-family:'IBM Plex Mono',monospace;font-size:0.75rem;font-weight:600;"
  "letter-spacing:0.08em;text-transform:uppercase;border:none;border-radius:2px;"
  "padding:10px 18px;cursor:pointer;transition:all 0.15s;white-space:nowrap}"
  ".btn-green{background:#4CAF6E;color:#0F1410}"
  ".btn-green:hover{background:#7DDB9A}"
  ".btn-amber{background:#7A4E10;color:#D4882A;border:1px solid #7A4E10}"
  ".btn-amber:hover{background:#D4882A;color:#0F1410}"
  ".btn-ghost{background:transparent;color:#5A7A5E;border:1px solid #2A3D2E}"
  ".btn-ghost:hover{border-color:#3D5C42;color:#C8DCC8}"
  ".status{font-size:0.75rem;margin-top:8px;min-height:18px;color:#5A7A5E}"
  ".status.ok{color:#7DDB9A}"
  ".status.err{color:#C0504A}"
  ".back{display:inline-block;margin-top:24px;font-size:0.72rem;color:#5A7A5E;"
  "text-decoration:none;border:1px solid #2A3D2E;padding:6px 14px;border-radius:2px}"
  ".back:hover{color:#C8DCC8;border-color:#3D5C42}"
  "</style></head><body>"
  "<h1>&#9881; TOOLS</h1>"
  "<div class=\"sub\">brausteuerung.local/tools</div>"

  "<h2>&#x2B06; index.html aktualisieren</h2>"
  "<p>Neue Web-Oberfl&auml;che hochladen ohne USB. Wird in LittleFS gespeichert "
  "und sofort aktiv. Mit <b>Reset</b> zur&uuml;ck auf die eingebettete Version.</p>"
  "<div class=\"row\">"
  "<input type=\"file\" id=\"htmlFile\" accept=\".html\">"
  "<button class=\"btn btn-green\" onclick=\"uploadHtml()\">&#x2B06; Upload</button>"
  "<button class=\"btn btn-ghost\" onclick=\"htmlReset()\">&#x21BA; Reset</button>"
  "</div>"
  "<div class=\"status\" id=\"htmlSt\"></div>"

  "<h2>&#x26A1; Firmware OTA Update</h2>"
  "<p>Arduino IDE: <b>Sketch &rarr; Exportiere kompilierte Bin&auml;rdatei</b><br>"
  "Dann die <b>.bin</b> Datei hier hochladen. ESP startet automatisch neu.</p>"
  "<div class=\"row\">"
  "<input type=\"file\" id=\"fwFile\" accept=\".bin\">"
  "<button class=\"btn btn-amber\" onclick=\"uploadFw()\">&#x26A1; Flashen</button>"
  "</div>"
  "<div class=\"status\" id=\"fwSt\"></div>"

  "<h2>&#x1F4CA; System Info</h2>"
  "<div id=\"sysinfo\" style=\"font-size:0.78rem;color:#5A7A5E;line-height:2\">Lade...</div>"

  "<a href=\"/\" class=\"back\">&#x2190; Zur&uuml;ck zur Steuerung</a>"

  "<script>"
  "async function uploadHtml(){"
  "const f=document.getElementById('htmlFile').files[0];"
  "if(!f)return;"
  "const st=document.getElementById('htmlSt');"
  "st.className='status';st.textContent='⏳ Lade hoch...';"
  "const form=new FormData();form.append('file',f);"
  "try{"
  "const r=await fetch('/upload/html',{method:'POST',body:form});"
  "const j=await r.json();"
  "if(j.ok){st.className='status ok';st.textContent='✓ '+j.msg;}"
  "else{st.className='status err';st.textContent='✗ '+(j.error||'Fehler');}"
  "}catch(e){st.className='status err';st.textContent='✗ Verbindungsfehler';}"
  "}"
  "async function htmlReset(){"
  "const r=await fetch('/upload/html-reset');"
  "const j=await r.json();"
  "const st=document.getElementById('htmlSt');"
  "st.className='status ok';st.textContent='✓ '+j.msg;"
  "}"
  "async function uploadFw(){"
  "const f=document.getElementById('fwFile').files[0];"
  "if(!f)return;"
  "const st=document.getElementById('fwSt');"
  "st.className='status';st.textContent='⏳ Flashe Firmware — bitte warten...';"
  "const form=new FormData();form.append('firmware',f);"
  "try{"
  "const r=await fetch('/update',{method:'POST',body:form,"
  "headers:{'Authorization':'Basic '+btoa('admin:brau2024')}});"
  "if(r.ok){st.className='status ok';st.textContent='✓ Update erfolgreich — ESP startet neu...';}"
  "else{st.className='status err';st.textContent='✗ Update fehlgeschlagen ('+r.status+')';}"
  "}catch(e){"
  "st.className='status ok';st.textContent='✓ ESP startet neu — bitte warten...';"
  "setTimeout(()=>location.href='/',5000);"
  "}"
  "}"
  "async function ladeSysinfo(){"
  "try{"
  "const r=await fetch('/api/sysinfo');const d=await r.json();"
  "document.getElementById('sysinfo').innerHTML="
  "'Heap frei: <b style=\"color:#7DDB9A\">'+( d.freeHeap/1024).toFixed(1)+' KB</b><br>'"
  "+'LittleFS: <b>'+d.fsUsed+' / '+d.fsTotal+' B</b><br>'"
  "+'Sketch: <b>'+(d.sketchSize/1024).toFixed(0)+' KB (frei: '+(d.freeSketch/1024).toFixed(0)+' KB)</b><br>'"
  "+'Uptime: <b>'+Math.floor(d.uptime/60)+' min</b><br>'"
  "+'HTML: <b style=\"color:#D4882A\">'+( d.htmlLittleFS?'LittleFS':'PROGMEM')+' </b>';"
  "}catch(e){document.getElementById('sysinfo').textContent='Fehler beim Laden';}"
  "}"
  "ladeSysinfo();"
  "</script></body></html>";

// ── HARDWARE ────────────────────────────────────────────────
OneWire           oneWire(PIN_DS18B20);
DallasTemperature sensors(&oneWire);
RCSwitch          rcSwitch;
ESP8266WebServer      server(80);
ESP8266HTTPUpdateServer otaServer;  // OTA Firmware Update

// OTA läuft gerade?
bool otaLaeuft = false;

// ── PID ─────────────────────────────────────────────────────
double pidInput    = 20.0;
double pidOutput   = 0.0;
double pidSetpoint = 67.0;
double Kp = 2.0, Ki = 0.5, Kd = 1.0;
PID myPID(&pidInput, &pidOutput, &pidSetpoint, Kp, Ki, Kd, DIRECT);

unsigned long PID_WINDOW_MS = 30000;  // konfigurierbar über Web-UI (in ms)
unsigned long pidWindowStart = 0;

// ── RCSWITCH ─────────────────────────────────────────────────
unsigned long rcCodeOn         = 0;
unsigned long rcCodeOff        = 0;
unsigned int  rcProtocol       = 1;
unsigned int  rcPulse          = 189;
unsigned int  rcBits           = 24;   // Bits im Code (meist 24)
unsigned int  rcWiederholungen = 10;   // Sendewiederholungen
bool          heizungAn        = false;
unsigned long letztesSenden    = 0;        // Zeitstempel letztes RC-Signal
const unsigned long RC_REPEAT_MS = 30000;  // alle 30s Signal wiederholen

// ── RASTEN ───────────────────────────────────────────────────
#define MAX_RASTEN 16

struct Rast {
  char  name[32];
  bool  on, halt, call;
  float sollTemp;
  int   time;
  float minTemp, maxTemp;
  float kp, ki, kd;
  float maxGradient;
  char  info[128];
};

Rast  rasten[MAX_RASTEN];
int   rastAnzahl   = 0;
int   aktiveRast   = -1;
bool  wartendHalt  = false;

unsigned long rastStartMs = 0;
unsigned long rastDauerMs = 0;

// ── ZUSTAND ──────────────────────────────────────────────────
enum Zustand { GESTOPPT, AUFHEIZEN, RAST_LAEUFT, WARTEN_HALT };
Zustand zustand = GESTOPPT;

// ── TEMPERATURSENSOR ─────────────────────────────────────────
float tempAktuell = 0.0;
float tempGradient = 0.0;          // °C/min
unsigned long letztesTempRead = 0;
const unsigned long TEMP_INTERVAL = 2000;

// ── TEMPERATUR RINGBUFFER ─────────────────────────────────────
// 1 Punkt alle 10s → 720 Punkte = 2h Verlauf
#define LOG_SIZE     720
#define LOG_INTERVAL 10000UL  // ms zwischen Logpunkten

struct LogPunkt {
  uint16_t sekundenSeitStart;  // Sekunden seit Braustart
  int16_t  temp100;            // Temperatur * 100 (z.B. 6723 = 67.23°C)
  int16_t  soll100;            // Solltemperatur * 100
  uint8_t  pidOut;             // PID Ausgang 0–100%
  uint8_t  rast;               // Aktive Rast-Nummer
};

LogPunkt  logBuffer[LOG_SIZE];
uint16_t  logAnzahl     = 0;
uint16_t  logKopf       = 0;    // Ringbuffer-Schreibzeiger
unsigned long logStartMs  = 0;
unsigned long letzterLogMs = 0;

void logReset() {
  logAnzahl  = 0;
  logKopf    = 0;
  logStartMs = millis();
  letzterLogMs = 0;
}

void logPunkt() {
  unsigned long jetzt = millis();
  if (jetzt - letzterLogMs < LOG_INTERVAL) return;
  letzterLogMs = jetzt;

  LogPunkt& p   = logBuffer[logKopf];
  p.sekundenSeitStart = (uint16_t)((jetzt - logStartMs) / 1000UL);
  p.temp100     = (int16_t)(tempAktuell * 100.0);
  p.soll100     = (int16_t)(pidSetpoint * 100.0);
  p.pidOut      = (uint8_t)constrain(pidOutput, 0, 100);
  p.rast        = (uint8_t)max(0, aktiveRast);

  logKopf = (logKopf + 1) % LOG_SIZE;
  if (logAnzahl < LOG_SIZE) logAnzahl++;
}

// ── BUZZER ───────────────────────────────────────────────────
void buzzerRuf(int n = 3) {
  for (int i = 0; i < n; i++) {
    digitalWrite(PIN_BUZZER, HIGH); delay(200);
    digitalWrite(PIN_BUZZER, LOW);  delay(200);
  }
}

// ── HEIZUNG ──────────────────────────────────────────────────
void rcSend(unsigned long code) {
  rcSwitch.setRepeatTransmit(rcWiederholungen);
  rcSwitch.send(code, rcBits);
}

void heizungEin() {
  unsigned long jetzt = millis();
  bool zustandsWechsel = !heizungAn;
  bool wiederholung    = (jetzt - letztesSenden >= RC_REPEAT_MS);
  if (rcCodeOn != 0 && (zustandsWechsel || wiederholung)) {
    rcSend(rcCodeOn);
    heizungAn    = true;
    letztesSenden = jetzt;
    Serial.printf("[RC] EIN  code=%lu bits=%d wdh=%d %s\n",
      rcCodeOn, rcBits, rcWiederholungen, wiederholung ? "(wdh)" : "");
  }
}
void heizungAus() {
  unsigned long jetzt = millis();
  bool zustandsWechsel = heizungAn;
  bool wiederholung    = (jetzt - letztesSenden >= RC_REPEAT_MS);
  if (rcCodeOff != 0 && (zustandsWechsel || wiederholung)) {
    rcSend(rcCodeOff);
    heizungAn    = false;
    letztesSenden = jetzt;
    Serial.printf("[RC] AUS  code=%lu bits=%d wdh=%d %s\n",
      rcCodeOff, rcBits, rcWiederholungen, wiederholung ? "(wdh)" : "");
  }
}

// ── CONFIG ───────────────────────────────────────────────────
void configLaden() {
  File f = LittleFS.open("/config.json", "r");
  if (!f) return;
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, f) == DeserializationError::Ok) {
    rcCodeOn         = doc["rcOn"]    | 0UL;
    rcCodeOff        = doc["rcOff"]   | 0UL;
    rcProtocol       = doc["rcProto"] | 1;
    rcPulse          = doc["rcPulse"] | 189;
    rcBits           = doc["rcBits"]  | 24;
    rcWiederholungen = doc["rcWdh"]   | 10;
    Kp               = doc["Kp"]        | 2.0;
    Ki               = doc["Ki"]        | 0.5;
    Kd               = doc["Kd"]        | 1.0;
    PID_WINDOW_MS    = (doc["pidFenster"] | 30) * 1000UL;
  }
  f.close();
  myPID.SetTunings(Kp, Ki, Kd);
  rcSwitch.setProtocol(rcProtocol);
  rcSwitch.setPulseLength(rcPulse);
  rcSwitch.setRepeatTransmit(rcWiederholungen);
  Serial.printf("[CONFIG] Kp=%.2f Ki=%.3f Kd=%.2f rcOn=%lu rcOff=%lu bits=%d wdh=%d\n",
                Kp, Ki, Kd, rcCodeOn, rcCodeOff, rcBits, rcWiederholungen);
}

void configSpeichern(const String& body) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) return;
  rcCodeOn         = doc["rcOn"]    | rcCodeOn;
  rcCodeOff        = doc["rcOff"]   | rcCodeOff;
  rcProtocol       = doc["rcProto"] | rcProtocol;
  rcPulse          = doc["rcPulse"] | rcPulse;
  rcBits           = doc["rcBits"]  | rcBits;
  rcWiederholungen = doc["rcWdh"]   | rcWiederholungen;
  Kp            = doc["Kp"]        | Kp;
  Ki            = doc["Ki"]        | Ki;
  Kd            = doc["Kd"]        | Kd;
  PID_WINDOW_MS = (doc["pidFenster"] | (int)(PID_WINDOW_MS/1000)) * 1000UL;
  myPID.SetTunings(Kp, Ki, Kd);
  rcSwitch.setProtocol(rcProtocol);
  rcSwitch.setPulseLength(rcPulse);
  rcSwitch.setRepeatTransmit(rcWiederholungen);
  File f = LittleFS.open("/config.json", "w");
  if (f) { serializeJson(doc, f); f.close(); }
  Serial.printf("[CONFIG] Gespeichert: Kp=%.2f Ki=%.3f Kd=%.2f\n", Kp, Ki, Kd);
}

// ── WLAN CONFIG ──────────────────────────────────────────────
String wlanSSID = "";
String wlanPassword = "";

bool wlanConfigLaden() {
  File f = LittleFS.open("/wlan.json", "r");
  if (!f) return false;
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return false; }
  f.close();
  wlanSSID     = doc["ssid"] | "";
  wlanPassword = doc["pw"]   | "";
  return wlanSSID.length() > 0;
}

void wlanConfigSpeichern(const String& ssid, const String& pw) {
  StaticJsonDocument<256> doc;
  doc["ssid"] = ssid;
  doc["pw"]   = pw;
  File f = LittleFS.open("/wlan.json", "w");
  if (f) { serializeJson(doc, f); f.close(); }
  Serial.printf("[WLAN] Config gespeichert: %s\n", ssid.c_str());
}

void wlanConfigLoeschen() {
  LittleFS.remove("/wlan.json");
  Serial.println("[WLAN] Config gelöscht");
}

// ── AP MODUS ROUTEN ───────────────────────────────────────────
void apRoutenSetup() {
  // AP-Seite ausliefern
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", AP_HTML);
  });

  // Netzwerkscan
  server.on("/ap/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // WLAN speichern + Neustart
  server.on("/ap/save", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"Kein Body\"}");
      return;
    }
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
      server.send(400, "application/json", "{\"error\":\"JSON Fehler\"}");
      return;
    }
    String ssid = doc["ssid"] | "";
    String pw   = doc["pw"]   | "";
    if (ssid.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"SSID leer\"}");
      return;
    }
    wlanConfigSpeichern(ssid, pw);
    server.send(200, "application/json", "{\"ok\":true}");
    delay(500);
    ESP.restart();
  });

  server.onNotFound([]() {
    // Im AP-Modus alles zur Startseite umleiten
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
}

// ── BML PARSER ───────────────────────────────────────────────
String xmlTagWert(const String& xml, const String& tag) {
  String open  = "<" + tag + ">";
  String close = "</" + tag + ">";
  int s = xml.indexOf(open);
  if (s < 0) return "";
  s += open.length();
  int e = xml.indexOf(close, s);
  if (e < 0) return "";
  return xml.substring(s, e);
}

bool bmlLaden() {
  File f = LittleFS.open("/rezept.bml", "r");
  if (!f) return false;
  String xml = f.readString();
  f.close();
  rastAnzahl = 0;
  int pos = 0;
  while (rastAnzahl < MAX_RASTEN) {
    int start = xml.indexOf("<rast>", pos);
    if (start < 0) break;
    int end = xml.indexOf("</rast>", start);
    if (end < 0) break;
    String block = xml.substring(start, end + 7);
    Rast& r = rasten[rastAnzahl];
    r.on = (xmlTagWert(block, "On") == "true");
    if (r.on) {
      xmlTagWert(block, "Name").toCharArray(r.name, sizeof(r.name));
      r.halt        = xmlTagWert(block, "Halt")       == "true";
      r.call        = xmlTagWert(block, "Call")       == "true";
      r.sollTemp    = xmlTagWert(block, "SollTemp")   .toFloat();
      r.time        = xmlTagWert(block, "Time")       .toInt();
      r.minTemp     = xmlTagWert(block, "MinTemp")    .toFloat();
      r.maxTemp     = xmlTagWert(block, "MaxTemp")    .toFloat();
      r.kp          = xmlTagWert(block, "Kp")         .toFloat();
      r.ki          = xmlTagWert(block, "Ki")         .toFloat();
      r.kd          = xmlTagWert(block, "Kd")         .toFloat();
      r.maxGradient = xmlTagWert(block, "MaxGradient").toFloat();
      xmlTagWert(block, "Info").toCharArray(r.info, sizeof(r.info));
      rastAnzahl++;
    }
    pos = end + 7;
  }
  return rastAnzahl > 0;
}

// ── RAST STARTEN ─────────────────────────────────────────────
void rastStarten(int nr) {
  if (nr < 0 || nr >= rastAnzahl) return;
  aktiveRast   = nr;
  wartendHalt  = false;
  zustand      = AUFHEIZEN;
  Rast& r      = rasten[nr];
  pidSetpoint  = r.sollTemp;
  rastDauerMs  = (unsigned long)r.time * 60UL * 1000UL;
  // Rast-spezifische PID-Parameter wenn gesetzt
  if (r.kp > 0) myPID.SetTunings(r.kp, r.ki, r.kd);
  else           myPID.SetTunings(Kp, Ki, Kd);
  pidWindowStart = millis();
  // Log nur beim allerersten Rast-Start zurücksetzen
  if (logStartMs == 0) logReset();
  Serial.printf("[RAST %d] %s | Soll: %.1f°C | %d min\n",
                nr, r.name, r.sollTemp, r.time);
}

void naechsteRast() {
  wartendHalt = false;
  int next = aktiveRast + 1;
  if (next < rastAnzahl) {
    rastStarten(next);
  } else {
    zustand    = GESTOPPT;
    aktiveRast = -1;
    heizungAus();
    buzzerRuf(5);
    Serial.println("[INFO] Brauprogramm abgeschlossen!");
  }
}

// ── TEMPERATUR LESEN ─────────────────────────────────────────
float tempHistorie[6] = {0};  // letzte 6 Messungen à 2s = 12s Fenster
uint8_t tempHistIdx = 0;

void tempLesen() {
  if (millis() - letztesTempRead < TEMP_INTERVAL) return;
  letztesTempRead = millis();
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C) {
    tempHistorie[tempHistIdx] = t;
    tempHistIdx = (tempHistIdx + 1) % 6;
    // Gradient: ältester vs neuester Wert über 12s → °C/min
    float oldest = tempHistorie[tempHistIdx];  // ältester Wert
    if (oldest > 0.0) {
      tempGradient = (t - oldest) / 12.0 * 60.0;  // °C/min
    }
    tempAktuell = t;
    pidInput    = t;
  }
  logPunkt();
}

// ── PID / HEIZUNG ────────────────────────────────────────────
void heizungRegeln() {
  if (zustand == GESTOPPT) {
    // Auch im Stopp-Zustand alle 30s AUS senden
    unsigned long jetzt = millis();
    if (rcCodeOff != 0 && (jetzt - letztesSenden >= RC_REPEAT_MS)) {
      rcSend(rcCodeOff);
      heizungAn     = false;
      letztesSenden = jetzt;
      Serial.printf("[RC] AUS (Standby) code=%lu\n", rcCodeOff);
    }
    return;
  }
  myPID.Compute();
  unsigned long jetzt   = millis();
  unsigned long fenster = jetzt - pidWindowStart;
  if (fenster >= PID_WINDOW_MS) {
    pidWindowStart += PID_WINDOW_MS;
    fenster = 0;
  }
  unsigned long einschaltMs = (unsigned long)(pidOutput / 100.0 * PID_WINDOW_MS);
  if (fenster < einschaltMs) heizungEin();
  else                        heizungAus();
}

// ── BRAU-LOGIK ───────────────────────────────────────────────
void brauLogik() {
  if (zustand == GESTOPPT || aktiveRast < 0) return;
  Rast& r = rasten[aktiveRast];
  unsigned long jetzt = millis();

  // ── AUFHEIZEN: Warte bis Solltemperatur erreicht ──────────
  if (zustand == AUFHEIZEN) {
    if ((pidSetpoint - tempAktuell) <= 0.5) {
      // Temperatur erreicht → Timer starten, kein Piepen
      zustand     = RAST_LAEUFT;
      rastStartMs = jetzt;
      Serial.printf("[START] Rast %d laeuft — %.1f°C erreicht\n",
                    aktiveRast, tempAktuell);
    }
    return;
  }

  // ── WARTEN AUF WEITER (nach Call) ──────────────────────────
  if (zustand == WARTEN_HALT) return;

  // ── RAST LÄUFT: Timer überwachen ───────────────────────────
  if (zustand == RAST_LAEUFT) {
    if (jetzt - rastStartMs >= rastDauerMs) {
      Serial.printf("[ENDE] Rast %d abgeschlossen\n", aktiveRast);
      if (r.call) {
        // Call=true: Piepen + warten auf Weiter-Bestätigung
        buzzerRuf(3);
        zustand     = WARTEN_HALT;
        wartendHalt = true;
        Serial.printf("[CALL] Rast %d: warte auf Weiter\n", aktiveRast);
      } else {
        // Call=false: direkt zur nächsten Rast
        naechsteRast();
      }
    }
  }
}

// ── API: STATUS ──────────────────────────────────────────────
void apiStatus() {
  StaticJsonDocument<768> doc;
  doc["temp"]       = tempAktuell;
  doc["soll"]       = pidSetpoint;
  doc["gradient"]   = tempGradient;
  doc["pidOut"]     = pidOutput;
  doc["heizung"]    = heizungAn;
  doc["zustand"]    = (int)zustand;
  doc["rast"]       = aktiveRast;
  doc["rastAnzahl"] = rastAnzahl;
  doc["wartHalt"]   = wartendHalt;
  if (aktiveRast >= 0 && aktiveRast < rastAnzahl) {
    Rast& r = rasten[aktiveRast];
    doc["rastName"]  = r.name;
    doc["rastInfo"]  = r.info;
    doc["rastSoll"]  = r.sollTemp;
    doc["rastDauer"] = r.time;
    if (zustand == RAST_LAEUFT) {
      unsigned long verg   = (millis() - rastStartMs) / 1000;
      unsigned long gesamt = rastDauerMs / 1000;
      doc["timerVergangen"] = verg;
      doc["timerGesamt"]    = gesamt;
      doc["timerRest"]      = (gesamt > verg) ? (gesamt - verg) : 0;
    }
  }
  JsonArray liste = doc.createNestedArray("rasten");
  for (int i = 0; i < rastAnzahl; i++) {
    JsonObject o = liste.createNestedObject();
    o["nr"]   = i;
    o["name"] = rasten[i].name;
    o["soll"] = rasten[i].sollTemp;
    o["time"] = rasten[i].time;
    o["halt"] = rasten[i].halt;
    o["call"] = rasten[i].call;
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// ── API: TEMPERATUR LOG ──────────────────────────────────────
void apiLog() {
  // Liefert alle Logpunkte als kompaktes JSON
  // Ringbuffer chronologisch ausgeben
  String json = "{\"n\":" + String(logAnzahl) +
                ",\"interval\":" + String(LOG_INTERVAL/1000) +
                ",\"d\":[";
  uint16_t start = (logAnzahl < LOG_SIZE) ? 0 : logKopf;
  for (uint16_t i = 0; i < logAnzahl; i++) {
    uint16_t idx = (start + i) % LOG_SIZE;
    LogPunkt& p = logBuffer[idx];
    if (i > 0) json += ",";
    json += "[" + String(p.sekundenSeitStart) + ","
               + String(p.temp100) + ","
               + String(p.soll100) + ","
               + String(p.pidOut)  + ","
               + String(p.rast)    + "]";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void apiLogReset() {
  logReset();
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── API: STEUERUNG ───────────────────────────────────────────
void apiStart() {
  if (rastAnzahl == 0) {
    server.send(400, "application/json", "{\"error\":\"Kein Rezept geladen\"}");
    return;
  }
  int von = 0;
  if (server.hasArg("plain")) {
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    von = doc["von"] | 0;
  }
  rastStarten(von);
  server.send(200, "application/json", "{\"ok\":true}");
}

void apiStop() {
  zustand    = GESTOPPT;
  aktiveRast = -1;
  heizungAus();
  server.send(200, "application/json", "{\"ok\":true}");
}

void apiWeiter() {
  if (wartendHalt) {
    wartendHalt = false;
    Serial.printf("[WEITER] Rast %d bestätigt — nächste Rast\n", aktiveRast);
    naechsteRast();  // Weiter = nächste Rast starten
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── API: CONFIG ──────────────────────────────────────────────
void apiConfigGet() {
  StaticJsonDocument<256> doc;
  doc["rcOn"]    = rcCodeOn;
  doc["rcOff"]   = rcCodeOff;
  doc["rcProto"] = rcProtocol;
  doc["rcPulse"] = rcPulse;
  doc["rcBits"]  = rcBits;
  doc["rcWdh"]   = rcWiederholungen;
  doc["Kp"]         = Kp;
  doc["Ki"]         = Ki;
  doc["Kd"]         = Kd;
  doc["pidFenster"] = (int)(PID_WINDOW_MS / 1000);
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void apiConfigPost() {
  configSpeichern(server.arg("plain"));
  server.send(200, "application/json", "{\"ok\":true}");
}

void apiHeizungTest() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    bool an = doc["an"] | false;
    if (an) heizungEin(); else heizungAus();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── UPLOAD: BML ──────────────────────────────────────────────
File uploadFile;

void handleBMLUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("[UPLOAD] BML Start");
    LittleFS.remove("/rezept.bml");
    uploadFile = LittleFS.open("/rezept.bml", "w");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    Serial.printf("[UPLOAD] BML Ende: %u Bytes\n", upload.totalSize);
    bmlLaden();
    Serial.printf("[BML] %d Rasten geladen\n", rastAnzahl);
  }
}

void handleBMLUploadEnd() {
  if (rastAnzahl > 0)
    server.send(200, "application/json",
      "{\"ok\":true,\"rasten\":" + String(rastAnzahl) + "}");
  else
    server.send(500, "application/json",
      "{\"error\":\"BML konnte nicht geparst werden\"}");
}

// ── UPLOAD: CONFIG JSON ───────────────────────────────────────
void handleConfigUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("[UPLOAD] Config Start");
    LittleFS.remove("/config.json");
    uploadFile = LittleFS.open("/config.json", "w");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    Serial.printf("[UPLOAD] Config Ende: %u Bytes\n", upload.totalSize);
    configLaden();  // Sofort übernehmen
  }
}

void handleConfigUploadEnd() {
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── UPLOAD: INDEX.HTML ───────────────────────────────────────
void handleHtmlUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    // .gz oder .html erkennen
    bool isGz = upload.filename.endsWith(".gz");
    String fname = isGz ? "/index.html.gz" : "/index.html";
    Serial.printf("[UPLOAD] %s Start\n", fname.c_str());
    LittleFS.remove("/index.html");
    LittleFS.remove("/index.html.gz");
    uploadFile = LittleFS.open(fname, "w");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    Serial.printf("[UPLOAD] HTML Ende: %u Bytes\n", upload.totalSize);
  }
}

void handleHtmlUploadEnd() {
  bool ok = LittleFS.exists("/index.html") || LittleFS.exists("/index.html.gz");
  if (ok)
    server.send(200, "application/json", "{\"ok\":true,\"msg\":\"HTML gespeichert\"}");
  else
    server.send(500, "application/json", "{\"error\":\"Speichern fehlgeschlagen\"}");
}

void handleHtmlReset() {
  LittleFS.remove("/index.html");
  LittleFS.remove("/index.html.gz");
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"PROGMEM Version aktiv\"}");
  Serial.println("[HTML] LittleFS HTML geloescht — Setup-Seite aktiv");
}

// ── SETUP ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n[BOOT] Brausteuerung startet...");

  // ── GPIO0 / Flash-Knopf: Reset-Check ────────────────────
  // Kurz als Input lesen BEVOR Buzzer-Mode gesetzt wird
  pinMode(PIN_BUZZER, INPUT_PULLUP);
  delay(100);  // Entprellen
  bool resetGedrueckt = (digitalRead(PIN_BUZZER) == LOW);
  // Jetzt auf Output umschalten
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // ── LittleFS ────────────────────────────────────────────
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS Fehler!");
  } else {
    Serial.println("[FS] LittleFS OK");

    // Reset: WLAN-Config löschen
    if (resetGedrueckt) {
      Serial.println("[RESET] Flash-Knopf gehalten — lösche WLAN-Config!");
      wlanConfigLoeschen();
      // 3x kurz blinken als Bestätigung
      for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_BUZZER, HIGH); delay(100);
        digitalWrite(PIN_BUZZER, LOW);  delay(100);
      }
    }

    configLaden();
    bmlLaden();
    if (rastAnzahl > 0)
      Serial.printf("[BML] %d Rasten geladen\n", rastAnzahl);
  }

  // ── Sensoren & Hardware ──────────────────────────────────
  sensors.begin();
  Serial.printf("[TEMP] %d Sensor(en)\n", sensors.getDeviceCount());

  rcSwitch.enableTransmit(PIN_RCSWITCH);
  rcSwitch.setProtocol(rcProtocol);
  rcSwitch.setPulseLength(rcPulse);

  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 100);
  myPID.SetSampleTime(2000);

  // ── WLAN ────────────────────────────────────────────────
  WiFi.hostname(HOSTNAME);
  bool hatConfig = wlanConfigLaden();

  if (hatConfig) {
    // Gespeicherte Credentials → verbinden versuchen
    Serial.printf("[WIFI] Verbinde mit: %s\n", wlanSSID.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(wlanSSID.c_str(), wlanPassword.c_str());

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 30) {
      delay(500); Serial.print("."); tries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Verbunden: http://%s.local  (%s)\n",
                    HOSTNAME, WiFi.localIP().toString().c_str());
      MDNS.begin(HOSTNAME);
      apModus = false;
    } else {
      Serial.println("[WIFI] Verbindung fehlgeschlagen → AP-Modus");
      apModus = true;
    }
  } else {
    Serial.println("[WIFI] Keine Config → AP-Modus");
    apModus = true;
  }

  // AP-Modus starten falls nötig
  if (apModus) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Brausteuerung");  // Offen, kein Passwort
    Serial.printf("[AP] SSID: Brausteuerung (offen) | IP: %s\n",
                  WiFi.softAPIP().toString().c_str());
    // mDNS auch im AP-Modus starten
    MDNS.begin(HOSTNAME);
    Serial.printf("[mDNS] http://%s.local erreichbar\n", HOSTNAME);
    // Nur AP-Routen registrieren
    apRoutenSetup();
    server.begin();
    Serial.println("[HTTP] AP-Server gestartet — warte auf WLAN-Einrichtung");
    // Buzzer: langes Summen = AP-Modus
    digitalWrite(PIN_BUZZER, HIGH); delay(800); digitalWrite(PIN_BUZZER, LOW);
    return;  // Normales Setup überspringen
  }

  // ── NORMALE ROUTEN (WLAN verbunden) ─────────────────────
  server.on("/", HTTP_GET, []() {
    if (LittleFS.exists("/index.html.gz")) {
      File f = LittleFS.open("/index.html.gz", "r");
      server.sendHeader("Content-Encoding", "gzip");
      server.streamFile(f, "text/html");
      f.close();
    } else if (LittleFS.exists("/index.html")) {
      File f = LittleFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else {
      // Noch keine index.html — Setup-Seite anzeigen
      server.send_P(200, "text/html", SETUP_HTML);
    }
  });

  server.on("/api/status",       HTTP_GET,  apiStatus);
  server.on("/api/start",        HTTP_POST, apiStart);
  server.on("/api/stop",         HTTP_POST, apiStop);
  server.on("/api/weiter",       HTTP_POST, apiWeiter);
  server.on("/api/config",       HTTP_GET,  apiConfigGet);
  server.on("/api/config",       HTTP_POST, apiConfigPost);
  server.on("/api/heizung-test", HTTP_POST, apiHeizungTest);
  server.on("/api/log",          HTTP_GET,  apiLog);
  server.on("/api/log/reset",    HTTP_POST, apiLogReset);
  server.on("/tools", HTTP_GET, []() {
    server.send_P(200, "text/html", TOOLS_HTML);
  });

  server.on("/upload/bml",    HTTP_POST, handleBMLUploadEnd,    handleBMLUpload);
  server.on("/upload/config", HTTP_POST, handleConfigUploadEnd, handleConfigUpload);
  server.on("/upload/html",   HTTP_POST, handleHtmlUploadEnd,   handleHtmlUpload);
  server.on("/upload/html-reset", HTTP_GET, handleHtmlReset);

  // OTA Firmware Update unter /update (GET=Formular, POST=Upload)
  otaServer.setup(&server, "/update", "admin", "brau2024");
  Serial.println("[OTA] Firmware-Update verfügbar unter /update");

  // System-Info API
  server.on("/api/sysinfo", HTTP_GET, []() {
    StaticJsonDocument<256> doc;
    doc["freeHeap"]      = ESP.getFreeHeap();
    doc["flashSize"]     = ESP.getFlashChipSize();
    doc["sketchSize"]    = ESP.getSketchSize();
    doc["freeSketch"]    = ESP.getFreeSketchSpace();
    doc["uptime"]        = millis() / 1000;
    doc["version"]       = "1.0.0";
    doc["htmlLittleFS"]  = LittleFS.exists("/index.html");
    FSInfo fs;
    LittleFS.info(fs);
    doc["fsTotal"]       = fs.totalBytes;
    doc["fsUsed"]        = fs.usedBytes;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("[HTTP] Server gestartet");
  buzzerRuf(2);
  Serial.println("[BOOT] Bereit!");
}

// ── LOOP ─────────────────────────────────────────────────────
void loop() {
  server.handleClient();
  MDNS.update();
  tempLesen();
  heizungRegeln();
  brauLogik();
}
