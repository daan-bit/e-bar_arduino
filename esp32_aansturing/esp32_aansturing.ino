#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

const char *ssid = "e-bar";
const char *password =  "123456789";
const char *msg_toggle_led = "toggleLED";
const char *msg_get_led = "getLEDState";
const int dns_port = 53;
const int http_port = 80;
const int ws_port = 1337;

String drink = "";
String soda = "";
String device = "";
String inhoud = "";

boolean aanwezig = false;


DynamicJsonDocument doc(512);

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(1337);
char msg_buf[10];
int led_state = 0;

/***********************************************************
 * Functions
 */

// Callback functies
void onWebSocketEvent(uint8_t client_num,WStype_t type,uint8_t * payload, size_t length){

  // Bekijkt type inkomend bericht
  switch(type) {

    // Client disconnect
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;

    // Client connect
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;

    // Inkomend bericht van client
    case WStype_TEXT:

      // Print inkomend bericht
      Serial.printf("[%u] Received text: %s\n", client_num, payload);

      deserializeJson(doc, payload);
      //serializeJson(doc, Serial); print naar serieel (test)

      //Haalt device op, zet in variabele en print deze uit
      device = doc["device"].as<const char *>();
      Serial.println("Device: ");
      Serial.println(device);

      //Als de raspberry pi een bericht stuurt
      if(strcmp(device.c_str(), "raspberry") == 0){
        inhoud = doc["inhoud"].as<const char *>();
        Serial.println("Inhoud in ml: ");
        //Doorgegeven inhoud van glas wordt in variabele gezet
        Serial.println(inhoud);
        //Geeft seintje aan ESP dat hij door kan naar keuzeMenu pagina
        aanwezig = true;
        String redirect = "redirect";
        webSocket.sendTXT(client_num, redirect.c_str());
      }
      //Als de interface een bericht stuurt - mixdrank
      else if (strcmp(device.c_str(), "web") == 0){
        //Kijkt of er een glas aanwezig is
        if(aanwezig == true){
          //Zet drank en frisdrank in variabele en print die.
          Serial.print("Drink: ");
          drink = doc["drink"].as<const char *>();
          Serial.println(drink);
    
          Serial.print("Soda: ");
          soda = doc["soda"].as<const char *>();
          Serial.println(soda);

          //##################################
          //Hier komt de code voor het inschenken van een mixdrankje
          //##################################
          
        }else{
          //Als er geen glas staat - foutmelding.
          //Moet in eerste instantie niet op deze pagina kunnen komen, maar als soort extra beveiliging
          Serial.println("Zet eerst een glas neer");
        }

       //Als interface bericht stuurt - shot
      }else if (strcmp(device.c_str(), "shot") == 0){
        if(aanwezig == true){
          Serial.println("Dit is een shotje");

          //Zet drank in variabele en print die
          Serial.print("Soda: ");
          soda = doc["soda"].as<const char *>();
          Serial.println(soda);

          //##################################
          //Hier komt de code voor het inschenken van een shotje
          //##################################
          
        }else{
          //Als er geen glas staat - foutmelding.
          //Moet in eerste instantie niet op deze pagina kunnen komen, maar als soort extra beveiliging
          Serial.println("Zet eerst een glas neer");
        }
      }

      
      
      break;

    //Voor alle andere callback functies gebeurt niets.
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

void onPiRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [ks] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/pi.html", "text/html");
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

void onRandomRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/random.png", "image/png");
}

// 404: als pagina niet kan vinden
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
  //Start seriele verbinding
  Serial.begin(115200);

  // Kijkt of SPIFFS benaderd kan worden
  if( !SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }

  // Start access point
  WiFi.softAP(ssid, password);

  // Print het ip adres van acces point
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
  server.on("/pi", HTTP_GET, onPiRequest);

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
  server.on("/img/random.png", onRandomRequest);

  // Als pagina niet kan vinden
  server.onNotFound(onPageNotFound);

  // Start web server
  server.begin();

  // Start websocket server en verwijst door naar callback functies
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);  
  
}

void loop() {
  
  // Zorgt dat websocket data steeds afgehandeld wordt
  webSocket.loop();
}
