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
 *   - /log.csv      → Temperaturlog (wird beim Brauen geschrieben)
 *
 * Reset (per /tools → Komplett-Reset):
 *   Löscht alle LittleFS Dateien → Werksreset
 *
 * Version: 4.0.0
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

  "<h2>&#x1F4BE; Einstellungen Backup / Restore</h2>"
  "<p>Sichert oder stellt <b>config.json</b> wieder her (RCSwitch, PID, Einstellungen).</p>"
  "<div class=\"row\">"
  "<a href=\"/api/settings/backup\" download=\"brewbrick_config.json\" class=\"btn btn-green\">&#x2B07; Backup herunterladen</a>"
  "</div>"
  "<div class=\"section-title\" style=\"margin-top:16px\">Einstellungen wiederherstellen</div>"
  "<div class=\"row\">"
  "<input type=\"file\" id=\"cfgFile\" accept=\".json\">"
  "<button class=\"btn btn-amber\" onclick=\"restoreConfig()\">&#x2B06; Restore</button>"
  "</div>"
  "<div class=\"status\" id=\"cfgSt\"></div>"
  "<h2>&#x1F4F6; Komplett-Reset</h2>"
  "<p>L\u00f6scht <b>alle Dateien</b> im Flash (WLAN, Config, Rezept, Log, HTML) und startet neu im AP-Modus.<br>"
  "<b style=\"color:#C0504A\">Achtung: Alle Einstellungen gehen verloren!</b></p>"
  "<button class=\"btn btn-amber\" onclick=\"wlanReset()\">&#x26A0; Alles l\u00f6schen &amp; Neustart</button>"
  "<div id=\"wlanSt\" class=\"status\" style=\"margin-top:8px\"></div>"
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
  "async function restoreConfig(){"
  "const f=document.getElementById('cfgFile').files[0];"
  "if(!f)return;"
  "const st=document.getElementById('cfgSt');"
  "st.className='status';st.textContent='⏳ Wiederherstelle...';"
  "const form=new FormData();form.append('file',f);"
  "try{"
  "const r=await fetch('/upload/config',{method:'POST',body:form});"
  "const j=await r.json();"
  "if(j.ok){st.className='status ok';st.textContent='✓ Wiederhergestellt — neu laden';}"
  "else{st.className='status err';st.textContent='✗ Fehler';}"
  "}catch(e){st.className='status err';st.textContent='✗ Verbindungsfehler';}"
  "}"
  "async function wlanReset(){"
  "if(!confirm('WLAN-Config wirklich loeschen?')) return;"
  "const st=document.getElementById('wlanSt');"
  "st.textContent='Losche...';"
  "try{ await fetch('/api/wlan/reset',{method:'POST'}); }catch(e){}"
  "st.textContent='ESP startet neu...';"
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
double Kp = 5.0, Ki = 0.05, Kd = 10.0;
double pidSchwellwert = 5.0;  // °C unter Soll → 100% Leistung
PID myPID(&pidInput, &pidOutput, &pidSetpoint, Kp, Ki, Kd, DIRECT);

unsigned long PID_WINDOW_MS = 30000;  // konfigurierbar über Web-UI (in ms)
unsigned long pidWindowStart = 0;

// ── RCSWITCH ─────────────────────────────────────────────────
unsigned long rcCodeOn         = 1631343;
unsigned long rcCodeOff        = 1631342;
unsigned int  rcProtocol       = 1;
unsigned int  rcPulse          = 315;
unsigned int  rcBits           = 24;
unsigned int  rcWiederholungen = 15;
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
uint8_t sensorFehlerZaehler = 0;   // Zählt aufeinanderfolgende Fehlmessungen
bool    sensorFehler = false;       // true = Sensor getrennt
unsigned long letztesTempRead = 0;
const unsigned long TEMP_INTERVAL = 2000;

// ── FLASH LOG ────────────────────────────────────────────────
#define LOG_INTERVAL 10000UL  // ms zwischen Logpunkten
unsigned long logStartMs   = 0;
unsigned long letzterLogMs = 0;
bool          logAktiv     = false;  // nur wenn Brauvorgang läuft

void logReset() {
  logStartMs   = millis();
  letzterLogMs = 0;
  logAktiv     = false;
  LittleFS.remove("/log.csv");
  Serial.println("[LOG] Flash-Log gelöscht");
}

void logStart() {
  logReset();
  logAktiv = true;
  File f = LittleFS.open("/log.csv", "w");
  if (f) { f.println("Zeit_s;Temp_C;Soll_C;PID_Pct;Rast"); f.close(); }
  Serial.println("[LOG] Flash-Log gestartet");
}

void logPunkt() {
  if (!logAktiv) return;
  unsigned long jetzt = millis();
  if (jetzt - letzterLogMs < LOG_INTERVAL) return;
  letzterLogMs = jetzt;
  unsigned long sek = (jetzt - logStartMs) / 1000UL;
  File f = LittleFS.open("/log.csv", "a");
  if (f) {
    f.printf("%lu;%.2f;%.2f;%u;%d\n",
      sek, tempAktuell, pidSetpoint,
      (uint8_t)constrain(pidOutput, 0, 100),
      aktiveRast);
    f.close();
  }
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
    pidSchwellwert   = doc["pidSchwelle"]  | 5.0;
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
  PID_WINDOW_MS  = (doc["pidFenster"] | (int)(PID_WINDOW_MS/1000)) * 1000UL;
  pidSchwellwert = doc["pidSchwelle"] | pidSchwellwert;
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
  // PID zurücksetzen
  myPID.SetMode(MANUAL);
  pidOutput = 100.0;
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 100);
  // Log nur beim ersten Rast-Start beginnen
  if (!logAktiv) logStart();
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
    // Sensor OK
    if (sensorFehler) {
      sensorFehler = false;
      Serial.println("[SENSOR] Sensor wieder verbunden");
    }
    sensorFehlerZaehler = 0;
    tempHistorie[tempHistIdx] = t;
    tempHistIdx = (tempHistIdx + 1) % 6;
    float oldest = tempHistorie[tempHistIdx];
    if (oldest > 0.0) {
      tempGradient = (t - oldest) / 12.0 * 60.0;
    }
    tempAktuell = t;
    pidInput    = t;
  } else {
    // Sensor getrennt
    sensorFehlerZaehler++;
    if (sensorFehlerZaehler == 5) {  // 5 × 2s = 10s
      sensorFehler = true;
      heizungAus();
      Serial.println("[FEHLER] Sensor getrennt — Heizung gestoppt!");
      buzzerRuf(5);
    }
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
  // ── Schwellwert: unter (Soll - Schwellwert) → 100% Leistung
  if (pidSchwellwert > 0.0 && (pidSetpoint - pidInput) > pidSchwellwert) {
    pidOutput = 100.0;  // Volle Leistung bis Schwellwert erreicht
  } else {
    myPID.Compute();  // PID regelt ab Schwellwert
  }

  // ── MaxGradient berücksichtigen ──────────────────────────
  // Wenn aktive Rast einen MaxGradient > 0 hat, PID-Output begrenzen
  double pidOutputBegrenzt = pidOutput;
  if (aktiveRast >= 0 && aktiveRast < rastAnzahl) {
    float maxGrad = rasten[aktiveRast].maxGradient;
    if (maxGrad > 0.0) {
      // Aktueller Gradient zu hoch → Leistung reduzieren
      if (tempGradient > maxGrad) {
        // Linear zurückregeln: je mehr Überschreitung, desto weniger Leistung
        float ueberschreitung = tempGradient - maxGrad;
        float reduktion = ueberschreitung / maxGrad * 100.0;
        pidOutputBegrenzt = max(0.0, pidOutput - reduktion);
        Serial.printf("[GRAD] Gradient %.1f°C/min > Max %.1f°C/min — PID %0.f%% → %.0f%%\n",
          tempGradient, maxGrad, pidOutput, pidOutputBegrenzt);
      }
    }
    // maxGradient == 0 → keine Begrenzung, volle Leistung erlaubt
  }

  unsigned long jetzt   = millis();
  unsigned long fenster = jetzt - pidWindowStart;
  if (fenster >= PID_WINDOW_MS) {
    pidWindowStart += PID_WINDOW_MS;
    fenster = 0;
  }
  unsigned long einschaltMs = (unsigned long)(pidOutputBegrenzt / 100.0 * PID_WINDOW_MS);
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
  Serial.printf("[HTTP] GET /api/status — Client: %s\n",
    server.client().remoteIP().toString().c_str());
  StaticJsonDocument<1536> doc;  // 16 Rasten × ~70B + Overhead
  doc["temp"]       = tempAktuell;
  doc["soll"]       = pidSetpoint;
  doc["gradient"]   = tempGradient;
  doc["pidOut"]     = pidOutput;
  doc["heizung"]    = heizungAn;
  doc["zustand"]    = (int)zustand;
  doc["rast"]       = aktiveRast;
  doc["rastAnzahl"] = rastAnzahl;
  doc["wartHalt"]   = wartendHalt;
  doc["sensorFehler"] = sensorFehler;
  doc["logAktiv"]   = logAktiv;
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

// ── API: LOG (liest Flash-CSV → JSON für Chart) ─────────────
void apiLog() {
  if (!LittleFS.exists("/log.csv")) {
    server.send(200, "application/json", "{\"n\":0,\"interval\":10,\"d\":[]}");
    return;
  }
  File f = LittleFS.open("/log.csv", "r");
  String json = "{\"n\":0,\"interval\":10,\"d\":[";
  bool first = true;
  int count = 0;
  f.readStringUntil('\n');  // Header überspringen
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;
    int s1=line.indexOf(';'), s2=line.indexOf(';',s1+1);
    int s3=line.indexOf(';',s2+1), s4=line.indexOf(';',s3+1);
    if (s4 < 0) continue;
    long  t   = line.substring(0,s1).toInt();
    float tmp = line.substring(s1+1,s2).toFloat();
    float sol = line.substring(s2+1,s3).toFloat();
    int   pid = line.substring(s3+1,s4).toInt();
    int   rst = line.substring(s4+1).toInt();
    if (!first) json += ",";
    json += "[" + String(t) + "," +
            String((int)(tmp*100)) + "," +
            String((int)(sol*100)) + "," +
            String(pid) + "," +
            String(rst) + "]";
    first = false;
    count++;
    yield();
  }
  f.close();
  json += "]}";
  json.replace("{\"n\":0,", "{\"n\":" + String(count) + ",");
  server.send(200, "application/json", json);
}

void apiLogReset() {
  logReset();
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── API: FLASH LOG DOWNLOAD ──────────────────────────────────
void apiLogDownload() {
  if (!LittleFS.exists("/log.csv")) {
    server.send(404, "text/plain", "Kein Log vorhanden");
    return;
  }
  File f = LittleFS.open("/log.csv", "r");
  server.sendHeader("Content-Disposition", "attachment; filename=braulog.csv");
  server.streamFile(f, "text/csv");
  f.close();
}

// ── API: TEMPERATUR LOG (RAM) ─────────────────────────────────


// ── API: FILE LISTING ────────────────────────────────────────
void apiFiles() {
  String json = "[";
  bool first = true;
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    if (dir.fileName() == "wlan.json") continue;  // geschützt
    if (!first) json += ",";
    json += "{\"name\":\"" + dir.fileName() + "\"," +
            "\"size\":" + String(dir.fileSize()) + "}";
    first = false;
  }
  json += "]";
  server.send(200, "application/json", json);
}

// ── API: FILE DELETE ─────────────────────────────────────────
void apiFileDelete() {
  if (!server.hasArg("name")) {
    server.send(400, "application/json", "{\"error\":\"name fehlt\"}");
    return;
  }
  String name = "/" + server.arg("name");
  // Sicherheit: wlan.json nicht löschbar (würde AP-Modus auslösen)
  if (name == "/wlan.json") {
    server.send(403, "application/json", "{\"error\":\"wlan.json geschützt — WLAN-Reset nutzen\"}");
    return;
  }
  if (!LittleFS.exists(name)) {
    server.send(404, "application/json", "{\"error\":\"Datei nicht gefunden\"}");
    return;
  }
  LittleFS.remove(name);
  Serial.printf("[FS] Gelöscht: %s\n", name.c_str());
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── API: SETTINGS BACKUP ─────────────────────────────────────
void apiSettingsBackup() {
  if (!LittleFS.exists("/config.json")) {
    server.send(404, "text/plain", "Keine Einstellungen vorhanden");
    return;
  }
  File f = LittleFS.open("/config.json", "r");
  server.sendHeader("Content-Disposition", "attachment; filename=brewbrick_config.json");
  server.streamFile(f, "application/json");
  f.close();
}

// ── API: STEUERUNG ───────────────────────────────────────────
void apiStart() {
  Serial.printf("[HTTP] POST /api/start — Client: %s\n",
    server.client().remoteIP().toString().c_str());
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
  Serial.printf("[HTTP] POST /api/stop — Client: %s\n",
    server.client().remoteIP().toString().c_str());
  zustand    = GESTOPPT;
  aktiveRast = -1;
  heizungAus();
  server.send(200, "application/json", "{\"ok\":true}");
}

void apiWeiter() {
  Serial.printf("[HTTP] POST /api/weiter — Client: %s\n",
    server.client().remoteIP().toString().c_str());
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
  doc["pidFenster"]  = (int)(PID_WINDOW_MS / 1000);
  doc["pidSchwelle"] = pidSchwellwert;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void apiConfigPost() {
  Serial.printf("[HTTP] POST /api/config — Client: %s\n",
    server.client().remoteIP().toString().c_str());
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

  // ── GPIO0 / Flash-Knopf: als Output (Buzzer) ────────────
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // ── LittleFS ────────────────────────────────────────────
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS Fehler!");
  } else {
    Serial.println("[FS] LittleFS OK");

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
    Serial.printf("[HTTP] GET / — Client: %s\n",
      server.client().remoteIP().toString().c_str());
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
  server.on("/api/log",             HTTP_GET,  apiLog);
  server.on("/api/log/reset",       HTTP_POST, apiLogReset);
  server.on("/api/log/download",    HTTP_GET,  apiLogDownload);
  server.on("/api/settings/backup", HTTP_GET,  apiSettingsBackup);
  server.on("/api/files",           HTTP_GET,  apiFiles);
  server.on("/api/files/delete",    HTTP_POST, apiFileDelete);
  server.on("/api/rezept/loeschen", HTTP_POST, []() {
    // Alle Rasten deaktivieren + BML-Datei löschen
    rastAnzahl = 0;
    aktiveRast = -1;
    zustand    = GESTOPPT;
    heizungAus();
    LittleFS.remove("/rezept.bml");
    Serial.println("[REZEPT] Gelöscht");
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/wlan/reset",   HTTP_POST, []() {
    Serial.println("[WLAN] Reset per Web-UI — lösche alle Dateien und starte neu");
    server.send(200, "application/json", "{\"ok\":true}");
    delay(500);
    // Alle Dateien löschen
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      LittleFS.remove("/" + dir.fileName());
      Serial.printf("[FS] Gelöscht: %s\n", dir.fileName().c_str());
    }
    ESP.restart();
  });
  server.on("/tools", HTTP_GET, []() {
    Serial.printf("[HTTP] GET /tools — Client: %s\n",
      server.client().remoteIP().toString().c_str());
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

  // wlan.json explizit sperren
  server.on("/wlan.json", HTTP_GET, []() {
    server.send(403, "text/plain", "Forbidden");
  });

  server.onNotFound([]() {
    Serial.printf("[HTTP] 404 %s — Client: %s\n",
      server.uri().c_str(), server.client().remoteIP().toString().c_str());
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("[HTTP] Server gestartet");
  buzzerRuf(2);
  Serial.println("[BOOT] Bereit!");
}

// ── LOOP ─────────────────────────────────────────────────────
unsigned long letztesTempLog = 0;
void loop() {
  server.handleClient();
  MDNS.update();
  tempLesen();
  heizungRegeln();
  brauLogik();
  // Temperatur alle 30s ins Serial loggen
  if (millis() - letztesTempLog >= 30000) {
    letztesTempLog = millis();
    Serial.printf("[TEMP] Ist: %.1f°C | Soll: %.1f°C | Gradient: %.1f°C/min | PID: %.0f%% | Heap: %u B\n",
      tempAktuell, pidSetpoint, tempGradient, pidOutput, ESP.getFreeHeap());
  }
}
