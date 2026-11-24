#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <time.h>

const char* ssid = "Ollie";
const char* password = "olliehotspot";
const char* mdnsName = "lifeplug";

// Belfast time (GMT/BST with automatic daylight saving)
const char* TZ_INFO = "GMT0BST,M3.5.0/1,M10.5.0";

const int relayPin = 26;
const bool RELAY_ACTIVE_LOW = true;

AsyncWebServer server(80);
Preferences prefs;

// Schedule: times in minutes since midnight, days as a bitmask (bit 0 = Sunday ... bit 6 = Saturday)
volatile bool scheduleEnabled = false;
volatile int onMinutes = 0;
volatile int offMinutes = 0;
volatile int dayMask = 0;
bool relayState = false;

void relaySet(bool on) {
  relayState = on;
  digitalWrite(relayPin, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

String fmt(int m) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", m / 60, m % 60);
  return String(buf);
}

String daysText(int mask) {
  if (mask == 0x7F) return "every day";
  const char* names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  String s = "";
  for (int i = 1; i <= 7; i++) {          // print Mon..Sun order
    int d = i % 7;
    if (mask & (1 << d)) {
      if (s.length()) s += ", ";
      s += names[d];
    }
  }
  return s;
}

String statusText() {
  String s = String(relayState ? "Charging" : "Lockout");
  if (scheduleEnabled) {
    s += " | Schedule: " + fmt(onMinutes) + " to " + fmt(offMinutes) + " (" + daysText(dayMask) + ")";
  } else {
    s += " | No schedule";
  }
  return s;
}

bool parseTime(const String& s, int &minutes) {
  int h, m;
  if (sscanf(s.c_str(), "%d:%d", &h, &m) != 2) return false;
  if (h < 0 || h > 23 || m < 0 || m > 59) return false;
  minutes = h * 60 + m;
  return true;
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <meta name="theme-color" content="#ffffff">
  <style>
    :root {
      --bg: #ffffff;
      --card: #f5faf7;
      --border: #dfeae3;
      --green: #00a859;
      --green-dark: #007a40;
      --green-soft: #e3f6ea;
      --red: #e5393b;
      --red-soft: #fdeaea;
      --amber: #f2a100;
      --amber-soft: #fff4d9;
      --text: #14201a;
      --muted: #6b7f72;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body {
      margin: 0;
      padding: 16px 16px calc(24px + env(safe-area-inset-bottom));
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      text-align: center;
    }
    .app { max-width: 480px; margin: 0 auto; }
    .logo { width: 100%; max-width: 500px; height: auto; margin: 8px 0 4px; }

    h2 { font-size: 18px; font-weight: 600; margin: 8px 0 14px; color: var(--text); }
    h3 {
      font-size: 12px; font-weight: 700; letter-spacing: 0.12em;
      text-transform: uppercase; color: var(--green-dark);
      margin: 0 0 14px; text-align: left;
    }

    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 18px;
      padding: 18px;
      margin: 14px 0;
      box-shadow: 0 2px 10px rgba(20, 60, 35, 0.06);
    }

    /* ---- gas monitor (placeholder) ---- */
    .gas {
      padding: 16px;
      border-radius: 14px;
      border: 1.5px solid var(--border);
      background: #fff;
      text-align: left;
    }
    .gas-title { font-size: 17px; font-weight: 700; display: flex; align-items: center; gap: 10px; }
    .gas-title::before {
      content: ""; flex: none; width: 12px; height: 12px; border-radius: 50%;
      background: var(--muted);
    }
    .gas-msg { margin-top: 6px; font-size: 14px; line-height: 1.4; color: var(--muted); }
    .gas.clear  { background: var(--green-soft); border-color: var(--green); }
    .gas.clear .gas-title { color: var(--green-dark); }
    .gas.clear .gas-title::before { background: var(--green); box-shadow: 0 0 0 4px rgba(0,168,89,0.2); }
    .gas.notice { background: var(--amber-soft); border-color: var(--amber); }
    .gas.notice .gas-title { color: #8a5a00; }
    .gas.notice .gas-title::before { background: var(--amber); box-shadow: 0 0 0 4px rgba(242,161,0,0.25); }
    .gas.danger { background: var(--red-soft); border-color: var(--red); animation: pulse 1s infinite; }
    .gas.danger .gas-title { color: #a8202a; }
    .gas.danger .gas-msg { color: #a8202a; font-weight: 600; }
    .gas.danger .gas-title::before { background: var(--red); box-shadow: 0 0 0 4px rgba(229,57,59,0.3); }
    @keyframes pulse {
      0%, 100% { box-shadow: 0 0 0 0 rgba(229,57,59,0.35); }
      50%      { box-shadow: 0 0 0 8px rgba(229,57,59,0); }
    }
    .gas-readings { display: flex; gap: 10px; margin-top: 12px; }
    .gas-readings div {
      flex: 1; background: #fff; border: 1px solid var(--border);
      border-radius: 12px; padding: 10px 4px;
    }
    .gas-readings b { display: block; font-size: 17px; color: var(--text); }
    .gas-readings span { font-size: 11px; letter-spacing: 0.08em; color: var(--muted); }

    /* ---- status pill ---- */
    #status {
      display: flex; align-items: center; justify-content: center; gap: 10px;
      margin: 0; padding: 14px 16px;
      background: #fff; border: 1px solid var(--border); border-radius: 14px;
      color: var(--text); font-size: 15px; font-weight: 500; line-height: 1.4;
    }
    #status::before {
      content: ""; flex: none; width: 10px; height: 10px; border-radius: 50%;
      background: var(--muted);
    }
    #status.on  { background: var(--green-soft); border-color: var(--green); color: var(--green-dark); }
    #status.on::before  { background: var(--green); box-shadow: 0 0 0 4px rgba(0,168,89,0.2); }
    #status.off { background: var(--red-soft); border-color: var(--red); color: #a8202a; }
    #status.off::before { background: var(--red); box-shadow: 0 0 0 4px rgba(229,57,59,0.2); }

    .row { display: flex; gap: 10px; }

    button {
      flex: 1; font-family: inherit; font-size: 16px; font-weight: 600;
      padding: 15px 12px; border-radius: 14px; border: 1.5px solid transparent;
      cursor: pointer; transition: transform 0.08s ease, filter 0.15s ease;
    }
    button:active { transform: scale(0.96); filter: brightness(0.95); }
    button.charge  { background: var(--green); color: #fff; box-shadow: 0 4px 12px rgba(0,168,89,0.3); }
    button.lock    { background: var(--red); color: #fff; box-shadow: 0 4px 12px rgba(229,57,59,0.28); }
    button.primary { background: var(--green-dark); color: #fff; box-shadow: 0 4px 12px rgba(0,122,64,0.28); }
    button.muted   { background: #fff; color: var(--muted); border-color: var(--border); }

    .time-row {
      display: flex; align-items: center; justify-content: space-between;
      padding: 8px 0; font-size: 16px; font-weight: 500; text-align: left;
    }
    .time-row + .time-row { border-top: 1px solid var(--border); }
    input[type=time] {
      font-family: inherit; font-size: 18px; font-weight: 600; padding: 10px 12px;
      background: #fff; color: var(--green-dark);
      border: 1px solid var(--border); border-radius: 10px; outline: none;
    }
    input[type=time]:focus { border-color: var(--green); box-shadow: 0 0 0 3px rgba(0,168,89,0.15); }

    .days {
      display: flex; flex-wrap: wrap; justify-content: center;
      gap: 8px; margin: 16px 0 18px;
    }
    .days label { cursor: pointer; }
    .days input { position: absolute; opacity: 0; pointer-events: none; }
    .days span {
      display: inline-block; min-width: 52px; padding: 10px 0;
      border-radius: 999px; border: 1px solid var(--border);
      background: #fff; color: var(--muted);
      font-size: 14px; font-weight: 600; transition: all 0.15s ease;
    }
    .days input:checked + span {
      background: var(--green); border-color: var(--green); color: #fff;
      box-shadow: 0 3px 8px rgba(0,168,89,0.3);
    }
  </style>
</head>
<body>
  <div class="app">
    <img class="logo" src="logo">

    <h2>One LiFEplug device connected</h2>

        <div class="card">
      <h3 onclick="demoGas()">Gas monitor</h3>
      <div id="gasBanner" class="gas">
        <div class="gas-title" id="gasTitle">No sensors connected yet</div>
        <div class="gas-msg" id="gasMsg">Gas monitoring is not set up. Once the SCD41 (CO2) and BME688 (VOC) are connected, this will show whether a dangerous battery failure has been detected, and a likely source for minor events (candle, deodorant, etc).</div>
      </div>
      <div class="gas-readings">
        <div><b id="gCo2">-</b><span>CO2 (SCD41)</span></div>
        <div><b id="gVoc">-</b><span>VOC (BME688)</span></div>
      </div>
    </div>

    <div class="card">
      <h3>Status</h3>
      <p id="status">loading...</p>
    </div>

    <div class="card">
      <h3>Manual</h3>
      <div class="row">
        <button class="charge" onclick="send('/relay/on')">Charging</button>
        <button class="lock" onclick="send('/relay/off')">Lockout</button>
      </div>
    </div>

    <div class="card">
      <h3>Schedule</h3>
      <label class="time-row">Turn on at <input type="time" id="onTime" value="08:00"></label>
      <label class="time-row">Turn off at <input type="time" id="offTime" value="17:00"></label>

      <div class="days">
        <label><input type="checkbox" data-bit="1" checked><span>Mon</span></label>
        <label><input type="checkbox" data-bit="2" checked><span>Tue</span></label>
        <label><input type="checkbox" data-bit="3" checked><span>Wed</span></label>
        <label><input type="checkbox" data-bit="4" checked><span>Thu</span></label>
        <label><input type="checkbox" data-bit="5" checked><span>Fri</span></label>
        <label><input type="checkbox" data-bit="6"><span>Sat</span></label>
        <label><input type="checkbox" data-bit="0"><span>Sun</span></label>
      </div>

      <div class="row">
        <button class="primary" onclick="setSchedule()">Set schedule</button>
        <button class="muted" onclick="send('/schedule/clear')">Clear</button>
      </div>
    </div>
  </div>

  <script>
    function showStatus(t) {
      const el = document.getElementById('status');
      el.innerText = t;
      el.className = t.indexOf('Charging') === 0 ? 'on'
                    : t.indexOf('Lockout') === 0 ? 'off' : '';
    }
    function send(url) {
      fetch(url)
        .then(r => r.text())
        .then(showStatus)
        .catch(() => showStatus('error'));
    }
    function setSchedule() {
      const on = document.getElementById('onTime').value;
      const off = document.getElementById('offTime').value;
      if (!on || !off) { alert('Pick both times'); return; }
      let mask = 0;
      document.querySelectorAll('.days input').forEach(cb => {
        if (cb.checked) mask |= (1 << parseInt(cb.dataset.bit));
      });
      if (mask === 0) { alert('Pick at least one day'); return; }
      send('/schedule?on=' + on + '&off=' + off + '&days=' + mask);
    }

        // ---- Gas monitor placeholder demo (tap the "Gas monitor" heading) ----
    const GAS_DEMOS = [
      { cls: '',       title: 'No sensors connected yet',
        msg: 'Gas monitoring is not set up. Once the SCD41 (CO2) and BME688 (VOC) are connected, this will show whether a dangerous battery failure has been detected, and a likely source for minor events (candle, deodorant, etc).',
        co2: '-', voc: '-' },
      { cls: 'clear',  title: 'Air clear',
        msg: 'CO2 and VOC levels are normal.',
        co2: '450 ppm', voc: '25' },
      { cls: 'notice', title: 'Air quality notice',
        msg: 'VOC spike with steady CO2. Probably an aerosol, deodorant or alcohol-based product.',
        co2: '520 ppm', voc: '240' },
      { cls: 'notice', title: 'Air quality notice',
        msg: 'CO2 and VOC rising together. Probably a candle, incense or cooking.',
        co2: '1100 ppm', voc: '130' },
      { cls: 'danger', title: 'DANGER: possible battery failure',
        msg: 'Rapid VOC and CO2 rise matches battery venting. Charging cut. Keep clear and ventilate.',
        co2: '2800 ppm', voc: '400+' }
    ];
    let demoIdx = 0;
    function demoGas() {
      demoIdx = (demoIdx + 1) % GAS_DEMOS.length;
      const d = GAS_DEMOS[demoIdx];
      document.getElementById('gasBanner').className = 'gas ' + d.cls;
      document.getElementById('gasTitle').textContent = d.title;
      document.getElementById('gasMsg').textContent = d.msg;
      document.getElementById('gCo2').textContent = d.co2;
      document.getElementById('gVoc').textContent = d.voc;
    }

    send('/status');
    setInterval(() => send('/status'), 10000);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  relaySet(false);

  // Load saved schedule
  prefs.begin("relay", false);
  scheduleEnabled = prefs.getBool("enabled", false);
  onMinutes = prefs.getInt("on", 0);
  offMinutes = prefs.getInt("off", 0);
  dayMask = prefs.getInt("days", 0x7F);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  MDNS.begin(mdnsName);

  configTzTime(TZ_INFO, "pool.ntp.org", "time.google.com");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  server.on("/logo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("https://raw.githubusercontent.com/ollie3951/ELE3044-Team-3/main/Subsystem%20Testing/User_Notification/Webserver/data/LiFE%20ARMOR%20Logo.png");
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", statusText());
  });

  server.on("/relay/on", HTTP_GET, [](AsyncWebServerRequest *request){
    scheduleEnabled = false;
    prefs.putBool("enabled", false);
    relaySet(true);
    request->send(200, "text/plain", statusText());
  });

  server.on("/relay/off", HTTP_GET, [](AsyncWebServerRequest *request){
    scheduleEnabled = false;
    prefs.putBool("enabled", false);
    relaySet(false);
    request->send(200, "text/plain", statusText());
  });

  server.on("/schedule", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("on") || !request->hasParam("off") || !request->hasParam("days")) {
      request->send(400, "text/plain", "Missing on/off/days");
      return;
    }
    int on, off;
    if (!parseTime(request->getParam("on")->value(), on) ||
        !parseTime(request->getParam("off")->value(), off)) {
      request->send(400, "text/plain", "Bad time format");
      return;
    }
    if (on == off) {
      request->send(400, "text/plain", "On and off times must differ");
      return;
    }
    int days = request->getParam("days")->value().toInt() & 0x7F;
    if (days == 0) {
      request->send(400, "text/plain", "Pick at least one day");
      return;
    }
    onMinutes = on;
    offMinutes = off;
    dayMask = days;
    scheduleEnabled = true;
    prefs.putInt("on", on);
    prefs.putInt("off", off);
    prefs.putInt("days", days);
    prefs.putBool("enabled", true);
    request->send(200, "text/plain", statusText());
  });

  server.on("/schedule/clear", HTTP_GET, [](AsyncWebServerRequest *request){
    scheduleEnabled = false;
    prefs.putBool("enabled", false);
    request->send(200, "text/plain", statusText());
  });

  server.begin();
}

void loop() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 1000) return;
  lastCheck = millis();

  if (!scheduleEnabled) return;

  struct tm t;
  if (!getLocalTime(&t, 0)) return;   // time not synced yet

  int now = t.tm_hour * 60 + t.tm_min;
  int today = t.tm_wday;               // 0 = Sunday
  int yesterday = (today + 6) % 7;

  bool todayOn = dayMask & (1 << today);
  bool yesterdayOn = dayMask & (1 << yesterday);

  bool shouldBeOn;
  if (onMinutes < offMinutes) {
    // Same-day window, e.g. 08:00 to 17:00
    shouldBeOn = todayOn && now >= onMinutes && now < offMinutes;
  } else {
    // Overnight window, e.g. 22:00 to 06:00
    // Evening part belongs to today's day; the after-midnight part belongs to yesterday's
    shouldBeOn = (todayOn && now >= onMinutes) || (yesterdayOn && now < offMinutes);
  }

  if (shouldBeOn != relayState) relaySet(shouldBeOn);
}