#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

// Constants
const char *ssid = "e-bar";
const char *password =  "123456789";
const char *msg_toggle_led = "toggleLED";
const char *msg_get_led = "getLEDState";
const int dns_port = 53;
const int http_port = 80;
const int ws_port = 1337;
const int led_pin = 15;

// Globals
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(1337);
char msg_buf[10];
int led_state = 0;

/***********************************************************
 * Functions
 */

// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num,WStype_t type,uint8_t * payload, size_t length){

  // Figure out the type of WebSocket event
  switch(type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;

    // Handle text messages from client
    case WStype_TEXT:

      // Print out raw message
      Serial.printf("[%u] Received text: %s\n", client_num, payload);

//      if ( strcmp((char *)payload, "toggleLED") == 0 ) {
//      // Toggle LED
//        led_state = led_state ? 0 : 1;
//        Serial.printf("Toggling LED to %u\n", led_state);
//        digitalWrite(led_pin, led_state);
//
//      // Report the state of the LED
//      } else if ( strcmp((char *)payload, "getLEDState") == 0 ) {
//        sprintf(msg_buf, "%d", led_state);
//        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
//        webSocket.sendTXT(client_num, msg_buf);
//
//      // Message not recognized
//      } else {
//        Serial.println("[%u] Message not recognized");
//      }
//      break;

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

// ophalen webpagina's 
// #############################################################################

void onIndexRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [index] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/index.html", "text/html");
}

void onKeuzeMenuRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [km] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/keuzeMenu.html", "text/html");
}

void onKiesDrankRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [kd] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/kiesDrank.html", "text/html");
}

void onKiesFrisRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [kf] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/kiesFris.html", "text/html");
}

void onKiesMixRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [kiesMix] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/kiesMix.html", "text/html");
}

void onLadenRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [l] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/laden.html", "text/html");
}

void onAfgehandeldRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [A] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/afgehandeld.html", "text/html");
}

void onFoutmeldingRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [f] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/foutmelding.html", "text/html");
}

void onKiesShotRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [ks] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/kiesShot.html", "text/html");
}

// Ophalen css / js / afbeeldingen
// #############################################################################

void onCSSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [style] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/style.css", "text/css");
}

void onJSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [js] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/main.js", "text/js");
}

void onAchtergrondRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/achtergrond.jpg", "image/jpg");
}

void onBacardiRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/bacardi.png", "image/png");
}

void onMalibuRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/malibu.png", "image/png");
}

void onVodkaRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/vodka.png", "image/png");
}

void onRocketRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/rocket.png", "image/png");
}

void onColaRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/cola.png", "image/png");
}

void onFantaRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/fanta.png", "image/png");
}

// Callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [404] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}

/***********************************************************
 * Main
 */

void setup() {
  // Start Serial port
  Serial.begin(115200);

  // Make sure we can read the file system
  if( !SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }

  // Start access point
  WiFi.softAP(ssid, password);

  // Print our IP address
  Serial.println();
  Serial.println("AP running");
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());

  // ophalen webpagina's 
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/keuzeMenu", HTTP_GET, onKeuzeMenuRequest);
  server.on("/kiesDrank", HTTP_GET, onKiesDrankRequest);
  server.on("/kiesFris", HTTP_GET, onKiesFrisRequest);
  server.on("/kiesMix", HTTP_GET, onKiesMixRequest);
  server.on("/laden", HTTP_GET, onLadenRequest);
  server.on("/afgehandeld", HTTP_GET, onAfgehandeldRequest);
  server.on("/foutmelding", HTTP_GET, onFoutmeldingRequest);
  server.on("/kiesShot", HTTP_GET, onKiesShotRequest);

  // Ophalen css / js / afbeeldingen
  server.on("/style.css", HTTP_GET, onCSSRequest);
  server.on("/main.js", HTTP_GET, onJSRequest);
  server.on("/img/achtergrond.jpg", onAchtergrondRequest);
  server.on("/img/bacardi.png", onBacardiRequest);
  server.on("/img/malibu.png", onMalibuRequest);
  server.on("/img/vodka.png", onVodkaRequest);
  server.on("/img/rocket.png", onRocketRequest);
  server.on("/img/cola.png", onColaRequest);
  server.on("/img/fanta.png", onFantaRequest);

  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);

  // Start web server
  server.begin();

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);  
  
}

void loop() {
  
  // Look for and handle WebSocket data
  webSocket.loop();
}
