#include <WiFi.h>
// #include <WebServer.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
// #include <LittleFS.h>

// ================== CHANGE THESE ==================
const char* ssid = "Ollie";
const char* password = "olliehotspot";

// mDNS name, what do we want the server to be called?
const char* mdnsName = "lifeplug";

// ==================================================

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h2>*Prototype Web Server*</h2>
  <img src="logo" style="max-width: 500px; width: 100%; height: auto;">
</body>  
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Wi-Fi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  // Start mDNS
  if (!MDNS.begin(mdnsName)) {
    Serial.println("mDNS start FAILED");
  } else {
    Serial.print("mDNS started: http://");
    Serial.print(mdnsName);
    Serial.println(".local/");
  }

  /*
  if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
  }
  */

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  server.on("/logo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("https://raw.githubusercontent.com/ollie3951/ELE3044-Team-3/main/Subsystem%20Testing/User_Notification/Webserver/data/LiFE%20ARMOR%20Logo.png");
  });

  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
}
