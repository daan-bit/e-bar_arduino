#include <ESP32Servo.h>
#include "BasicStepperDriver.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>


const char *ssid = "e-bar";
const char *password =  "123456789";
// const int dns_port = 53;
// const int http_port = 80;
// const int ws_port = 1337;

#define DNS_PORT 53
#define HTTP_PORT 80
#define WS_PORT 1337

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200
#define RPM 120

// Since microstepping is set externally, make sure this matches the selected mode
// If it doesn't, the motor will move at a different RPM than chosen
// 1=full step, 2=half step etc.
#define MICROSTEPS 16

#define MOTOR_ACCEL 750
#define MOTOR_DECEL 500

// All the wires needed for full functionality
#define DIR 15
#define STEP 2
#define SLEEP 13

// 2-wire basic config, microstepping is hardwired on the driver
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP, SLEEP);

#define SERVO_PIN_1 18
Servo servo1;
#define SERVO_PIN_2 25
Servo servo2;
#define SERVO_PIN_3 26
Servo servo3;
#define SERVO_PIN_4 27
Servo servo4;

#define SERVO_POS_MIN 0
#define SERVO_POS_MAX 180
#define SERVO_POS_IDLE 30

#define SERVO_1_START SERVO_POS_MIN
#define SERVO_2_START SERVO_POS_MIN
#define SERVO_3_START SERVO_POS_MAX
#define SERVO_4_START SERVO_POS_MAX

#define STEPPER_HOME 0
#define STEPPER_DISPENSER_POS_1 60
#define STEPPER_DISPENSER_POS_2 87
#define STEPPER_DISPENSER_POS_3 87
#define STEPPER_DISPENSER_POS_4 81

// #define PUMP_TIME_MS 17000
#define PUMP_MS_PER_ML 36

enum stpp {
  STPP_STEPPER_HOME = 0,
  STPP_STEPPER_DISPENSER_POS_1,
  STPP_STEPPER_DISPENSER_POS_2,
  STPP_STEPPER_DISPENSER_POS_3,
  STPP_STEPPER_DISPENSER_POS_4
};

enum drinks_e {
  DRINKS_N_VODKA = 0,
  DRINKS_N_MALIBU,
  DRINKS_N_ROCKETSHOT,
  DRINKS_N_BACARDI
};

struct drink_t {
  String name;
  bool alcohol;
  uint8_t deviceNum;
};

struct drinks_t {
  String name;
  drink_t drinks[2];
  bool isShot;
};

drinks_t drinks_NONE = { .name = "", .drinks = {}, .isShot = 0 };
drink_t drink_NONE = { .name = "", .alcohol = 0, .deviceNum = 0 };

// Non alcoholic
drink_t Cola = { .name = "Cola", .alcohol = false, .deviceNum = 5 };
drink_t Sinas = { .name = "Sinas", .alcohol = false, .deviceNum = 6 };
// Alcoholic
drink_t Malibu = { .name = "Malibu", .alcohol = true, .deviceNum = 1 };
drink_t Vodka = { .name = "Vodka", .alcohol = true, .deviceNum = 2 };
drink_t Bacardi = { .name = "Bacardi", .alcohol = true, .deviceNum = 3 };
drink_t RocketShot = { .name = "RocketShot", .alcohol = true, .deviceNum = 4 };

drinks_t drinks[] = {
  { .name = "Bacardi Cola", .drinks = { Bacardi, Cola }, .isShot = false },
  { .name = "Bacardi Sinas", .drinks = { Bacardi, Sinas }, .isShot = false },
  { .name = "Vodka Cola", .drinks = { Vodka, Cola }, .isShot = false },
  { .name = "Vodka Sinas", .drinks = { Vodka, Sinas }, .isShot = false },
  { .name = "Malibu Cola", .drinks = { Malibu, Cola }, .isShot = false },
  { .name = "Malibu Sinas", .drinks = { Malibu, Sinas }, .isShot = false },
  { .name = "Malibu", .drinks = { Malibu }, .isShot = true },
  { .name = "Vodka", .drinks = { Vodka }, .isShot = true },
  { .name = "Bacardi", .drinks = { Bacardi }, .isShot = true },
  { .name = "RocketShot", .drinks = { RocketShot }, .isShot = true }
};


#define PUMP_PIN_1 16
#define PUMP_PIN_2 17

#define ENDSTOP_PIN 22

uint8_t steps_per_mm = 90; // Preconfigured

uint16_t glass_ml = 300;

int stepperPos = 0;
int stepperStepsLeft = 0;

bool actions = false;
bool moving = false;
bool movedToPos = false;

uint8_t deviceNum = 0;
int argNum = 0;

drinks_t currentDrinks = drinks_NONE;
drink_t currentDrink = drink_NONE;
drink_t nextDrink = drink_NONE;


uint8_t currentPos[4] = {SERVO_1_START, SERVO_2_START, SERVO_3_START, SERVO_4_START};

bool endStopPressed = false;

DynamicJsonDocument doc(512);

AsyncWebServer server(HTTP_PORT);
WebSocketsServer webSocket = WebSocketsServer(WS_PORT);

void IRAM_ATTR endstop_interrupt() {
  endStopPressed = !digitalRead(ENDSTOP_PIN);
  // if (endStopPressed) {
  //   stepperPos = 0;
  // }
}

void setup() {
  Serial.begin(115200);
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo1.setPeriodHertz(50);    // standard 50 hz servo
  servo1.attach(SERVO_PIN_1, 500, 2500);
  servo2.setPeriodHertz(50);    // standard 50 hz servo
  servo2.attach(SERVO_PIN_2, 500, 2500);
  servo3.setPeriodHertz(50);    // standard 50 hz servo
  servo3.attach(SERVO_PIN_3, 400, 2600);
  servo4.setPeriodHertz(50);    // standard 50 hz servo
  servo4.attach(SERVO_PIN_4, 400, 2600);

  Serial.println("Configuring all pumps");
  pinMode(PUMP_PIN_1, OUTPUT);
  pinMode(PUMP_PIN_2, OUTPUT);
  Serial.println("All pumps configured!");

  Serial.println("Configuring end stop pin");
  pinMode(ENDSTOP_PIN, INPUT_PULLUP);
  endStopPressed = !digitalRead(ENDSTOP_PIN);
  attachInterrupt(ENDSTOP_PIN, endstop_interrupt, CHANGE);
  Serial.print("Pressed: ");
  Serial.println(endStopPressed);
  Serial.println("End stop pin configured!");

  Serial.println("Configuring stepper");
  stepper.begin(RPM, MICROSTEPS);
  stepper.setEnableActiveState(LOW);
  stepper.setSpeedProfile(stepper.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);
  stepper.enable();
  Serial.println("Stepper configured");

  Serial.println("Configuring all servo's, pleas wait..");
  servo1.write(SERVO_1_START);
  delay(500);
  servo2.write(SERVO_2_START);
  delay(500);
  servo3.write(SERVO_3_START);
  delay(500);
  servo4.write(SERVO_4_START);
  delay(500);
  Serial.println("All servo's configured!");

  if(!SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }

  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.println("AP running");
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Starting stepper calibration!");
  int r = calibrateStepper();
  Serial.print("Calibration result: ");
  Serial.println(r);
  Serial.println((steps_per_mm * 10 * 3) - (steps_per_mm * 10 * 2));
  steps_per_mm = r / 10;
  Serial.println("Calibration is done!");


  Serial.println("Setting up all webpages..");
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
  server.on("/games", HTTP_GET, onGamesRequest);
  server.on("/ki", HTTP_GET, onGameKingRequest);

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
  server.on("/img/reset.png", onResetRequest);
  server.on("/img/king.jpg", onKingRequest);

  // Als pagina niet kan vinden
  server.onNotFound(onPageNotFound);

  Serial.println("server.begin");
  // Start web server
  server.begin();

  Serial.println("webSocket.begin");
  // Start websocket server en verwijst door naar callback functies
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);


  bool debug = true;
  if (r < 0 && !debug) {
    Serial.println("ERROR ERROR ERROR ERROR ERROR ERROR");
    Serial.println(r);
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();
    while(true) {
      delay(50);
      yield();
      webSocket.broadcastTXT("1");
      webSocket.loop();
    }
  }

  // Temp
  disableCore0WDT();
  disableCore1WDT();
  disableLoopWDT();
}

bool cmpDrinks(drinks_t *d1, drinks_t *d2) {
  if (d1->name == d2->name) {
    return true;
  }
  return false;
}
bool cmpDrink(drink_t *d1, drink_t *d2) {
  if (d1->name == d2->name) {
    return true;
  }
  return false;
}

void loop() {
  webSocket.loop();
  unsigned wait_time_micros = stepper.nextAction();
  stepperStepsLeft = stepper.getStepsRemaining();
  if ((stepperStepsLeft > 0 && endStopPressed) && stepper.getDirection() == -1) {
    Serial.print("steps remaining: ");
    Serial.println(stepperStepsLeft);
    stepper.stop();
    Serial.print("steps remaining after stop: ");
    Serial.println(stepper.getStepsRemaining());
    moving = false;
  }
  if (stepperStepsLeft == 0) {
    moving = false;
    movedToPos = true;
  } else {
    moving = true;
    movedToPos = false;
  }

  String rec;
  while (Serial.available() > 0) {
    const char c = Serial.read();
    delay(5);
    rec += c;

    if (rec.length() > 0) actions = true;
  }


  if (!cmpDrinks(&currentDrinks, &drinks_NONE) && currentDrinks.isShot && cmpDrink(&currentDrink, &drink_NONE)) {
    Serial.println("Starting to make a shot! :D");
    currentDrink = currentDrinks.drinks[0];
    nextDrink = drink_NONE;
  } else if (!cmpDrinks(&currentDrinks, &drinks_NONE) && cmpDrink(&currentDrink, &drink_NONE) && cmpDrink(&nextDrink, &drink_NONE)) {
    currentDrink = currentDrinks.drinks[0];
    nextDrink = currentDrinks.drinks[1];
  }
  if (!cmpDrinks(&currentDrinks, &drinks_NONE) && !moving) {
    Serial.println("Making drink");
    Serial.println(stepperStepsLeft);
    // if (currentDrink == drink_NONE && nextDrink != drink_NONE) {
    //   currentDrink = nextDrink;
    //   nextDrink = drink_NONE;
    // } else if (currentDrink == drink_NONE && nextDrink == drink_NONE) {
    //   currentDrinks = drinks_NONE;
    //   return; // Done?
    // }
    if (cmpDrink(&currentDrink, &drink_NONE) && !cmpDrink(&nextDrink, &drink_NONE)) {
      Serial.println("Start next drink");
      currentDrink = nextDrink;
      nextDrink = drink_NONE;
    } else if (cmpDrink(&currentDrink, &drink_NONE) && cmpDrink(&nextDrink, &drink_NONE)) {
      Serial.println("|||||||||| Drink is done ||||||||||");
      currentDrinks = drinks_NONE;
      webSocket.broadcastTXT("0");
      return; // Done?
    }

    // if (currentDrink.deviceNum < 5) {
    //   switch(stepperPos) {
    //     case STEPPER_HOME:
    //       break;
    //     case STEPPER_HOME + STEPPER_DISPENSER_POS_1:
    //         currentPos[0] = rotate(&servo1, SERVO_POS_MAX, currentPos[0], 20);
    //         delay(1000);
    //         currentPos[0] = rotate(&servo1, SERVO_POS_IDLE, currentPos[0], 20);
    //         currentDrink = nextDrink;
    //         nextDrink = drink_NONE;
    //       break;
    //   }
    // }

    Serial.println("()()()()()()()");
    Serial.print("stepperPos = ");
    Serial.println(stepperPos);
    Serial.print("currentDrink = ");
    Serial.println(currentDrink.name);
    Serial.print("nextDrink = ");
    Serial.println(nextDrink.name);
    Serial.print("absPos = ");
    Serial.println(stepperAbsPos(STPP_STEPPER_DISPENSER_POS_4));
    Serial.println("()()()()()()()");

    if (stepperPos == stepperAbsPos(STPP_STEPPER_HOME) && (currentDrink.deviceNum == 5 || currentDrink.deviceNum == 6)) {
      actions = true;
      deviceNum = currentDrink.deviceNum;
      argNum = 0;
      currentDrinks = drinks_NONE;
      currentDrink = drink_NONE;
      webSocket.broadcastTXT("0");
    } else if (currentDrink.deviceNum == 5 || currentDrink.deviceNum == 6) {
      actions = true;
      deviceNum = 7;
      argNum = 0;
    } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_1) && currentDrink.deviceNum == 1) {
      currentPos[0] = rotate(&servo1, SERVO_POS_MAX, currentPos[0], 20);
      delay(1000);
      currentPos[0] = rotate(&servo1, SERVO_POS_IDLE, currentPos[0], 20);
      currentDrink = nextDrink;
      nextDrink = drink_NONE;
    } else if (currentDrink.deviceNum == 1) {
      actions = true;
      deviceNum = 7;
      argNum = 1;
    } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_2) && currentDrink.deviceNum == 2) {
      currentPos[1] = rotate(&servo2, SERVO_POS_MAX, currentPos[1], 20);
      delay(1100);
      currentPos[1] = rotate(&servo2, SERVO_POS_IDLE, currentPos[1], 20);
      currentDrink = nextDrink;
      nextDrink = drink_NONE;
    } else if (currentDrink.deviceNum == 2) {
      actions = true;
      deviceNum = 7;
      argNum = 2;
    } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_3) && currentDrink.deviceNum == 3) {
      currentPos[2] = rotate(&servo3, SERVO_POS_MIN, currentPos[2], 20);
      delay(1300);
      currentPos[2] = rotate(&servo3, 180 - SERVO_POS_MIN, currentPos[2], 20);
      currentDrink = nextDrink;
      nextDrink = drink_NONE;
    } else if (currentDrink.deviceNum == 3) {
      actions = true;
      deviceNum = 7;
      argNum = 3;
    } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_4) && currentDrink.deviceNum == 4) {
      Serial.println("Start moving servo");
      currentPos[3] = rotate(&servo4, SERVO_POS_MIN, currentPos[3], 20);
      delay(1300);
      currentPos[3] = rotate(&servo4, 180 - SERVO_POS_MIN, currentPos[3], 20);
      currentDrink = nextDrink;
      nextDrink = drink_NONE;
    } else if (currentDrink.deviceNum == 4) {
      Serial.println("[4] Not yet moved to the correct position.");
      actions = true;
      deviceNum = 7;
      argNum = 4;
    }


    Serial.println("*************");
    Serial.print("stepperPos = ");
    Serial.println(stepperPos);
    Serial.print("currentDrink = ");
    Serial.println(currentDrink.name);
    Serial.print("nextDrink = ");
    Serial.println(nextDrink.name);
    Serial.print("absPos = ");
    Serial.println(stepperAbsPos(STPP_STEPPER_DISPENSER_POS_4));
    Serial.println("*************");


    if (cmpDrink(&currentDrink, &drink_NONE) && currentDrinks.isShot == true && cmpDrink(&currentDrink, &drink_NONE)) {
      Serial.println("STOP MAKING MORE PLEASE");
      currentDrinks = drinks_NONE;
      nextDrink = drink_NONE;
      currentDrink = drink_NONE;
      actions = true;
      deviceNum = 7;
      argNum = 0;
      webSocket.broadcastTXT("0");
    }
    // if (movedToPos) {
    //   actions = true;
    //   movedToPos = false;
    //   switch(currentDrink.deviceNum) {
    //     case 1:
    //       deviceNum = 7;
    //       argNum = 1;
    //       break;
    //     case 2:
    //       deviceNum = 7;
    //       argNum = 2;
    //       break;
    //     case 3:
    //       deviceNum = 7;
    //       argNum = 3;
    //       break;
    //     case 4:
    //       deviceNum = 7;
    //       argNum = 4;
    //       break;
    //     case 5:
    //       deviceNum = 5;
    //       argNum = 0;
    //       break;
    //     case 6:
    //       // TODO PUMP 2
    //       break;
    //     default:
    //       actions = false;
    //       break;
    //   }
    // }
  }


  if (!actions) {
    return;
  }
  actions = false;

  Serial.println("Received");
  Serial.println(rec);

  if (rec.length() > 0) {
    uint8_t devCharLen = 0;
    for (uint8_t i = 0; i < rec.length(); i++) {
      if (isSpace(rec.charAt(i))) {
        Serial.println("Found space!");
        devCharLen = i - 1;
        break;
      }
    }
    Serial.print("devCharLen:: ");
    Serial.println(devCharLen);
    deviceNum = (uint8_t)(rec.substring(0, devCharLen + 1).toInt());
    argNum = rec.substring(devCharLen + 1).toInt();
  }

  Serial.print("device num:: ");
  Serial.println(deviceNum);
  Serial.print("argNum:: ");
  Serial.println(argNum);

  uint8_t servoSpeed = 20;

  switch (deviceNum) {
    case 1:
      Serial.println("Servo 1");
      if (currentPos[0] > argNum) {
        servoSpeed = 15;
      }
      currentPos[0] = rotate(&servo1, argNum, currentPos[0], 20);
      servoSpeed = 20;
      break;
    case 2:
      Serial.println("Servo 2");
      currentPos[1] = rotate(&servo2, argNum, currentPos[1], 20);
      break;
    case 3:
      Serial.println("Servo 3");
      currentPos[2] = rotate(&servo3, argNum, currentPos[2], 20);
      break;
    case 4:
      Serial.println("Servo 4");
      currentPos[3] = rotate(&servo4, argNum, currentPos[3], 20);
      break;
    case 5:
      Serial.println("Pump 1");
      stepper.disable();
      for (uint16_t i = 0; i < PUMP_MS_PER_ML * glass_ml / 30; i++) {
        analogWrite(PUMP_PIN_1, 255);
        delay(20);
        analogWrite(PUMP_PIN_1, 100);
        delay(10);
      }
      analogWrite(PUMP_PIN_1, 0);
      // digitalWrite(PUMP_PIN_1, HIGH);
      // delay(PUMP_MS_PER_ML * glass_ml);
      // digitalWrite(PUMP_PIN_1, LOW);
      stepper.enable();
      // analogWrite(PUMP_PIN_1, 0);
      Serial.println("Pump 1 end");
      break;
    case 6: // Pump 2
      Serial.println("Pump 2");
      stepper.disable();
      for (uint16_t i = 0; i < PUMP_MS_PER_ML * glass_ml / 30; i++) {
        analogWrite(PUMP_PIN_2, 255);
        delay(20);
        analogWrite(PUMP_PIN_2, 100);
        delay(10);
      }
      analogWrite(PUMP_PIN_2, 0);
      // digitalWrite(PUMP_PIN_2, HIGH);
      // delay(PUMP_MS_PER_ML * glass_ml);
      // digitalWrite(PUMP_PIN_2, LOW);
      stepper.enable();
      Serial.println("Pump 2 end");
      // Serial.println("Stepper move");
      // Serial.print("Stepper move this much: ");
      // Serial.println(steps_per_mm * argNum);
      // stepper.startMove(steps_per_mm * argNum);
      // // delay(1000);
      // // stepper.rotate(-90);
      // Serial.print("direction: ");
      // Serial.println(stepper.getDirection());
      // Serial.println("Stepper move END");
      break;
    case 7:
      Serial.print("stepperPos before:: ");
      Serial.println(stepperPos);
      switch(argNum) {
        case 0:
          Serial.print("StepperMoveAmount::::::::::::::::::::::::::: ");
          Serial.println(steps_per_mm * stepperMoveAmount(stepperPos, STPP_STEPPER_HOME));
          stepper.startMove(steps_per_mm * stepperMoveAmount(stepperPos, STPP_STEPPER_HOME));
          stepperPos = stepperAbsPos(STPP_STEPPER_HOME);
          // moving = true;
          // stepperStepsLeft += 1;
          // Start motor here
          // TEMP
          // actions = true;
          // deviceNum = 5;
          // argNum = 0;
          // END TEMP (remove later)
          // currentDrinks = drinks_NONE;
          // currentDrink = drink_NONE;
          break;
        case 1:
          Serial.print("StepperMoveAmount1:: ");
          Serial.println(stepperMoveAmount(stepperPos, STPP_STEPPER_DISPENSER_POS_1));
          stepper.startMove(steps_per_mm * stepperMoveAmount(stepperPos, STPP_STEPPER_DISPENSER_POS_1));
          stepperPos = stepperAbsPos(STPP_STEPPER_DISPENSER_POS_1);
          stepperStepsLeft += 1;
          moving = true;
          break;
        case 2:
          stepper.startMove(steps_per_mm * stepperMoveAmount(stepperPos, STPP_STEPPER_DISPENSER_POS_2));
          stepperPos = stepperAbsPos(STPP_STEPPER_DISPENSER_POS_2);
          stepperStepsLeft += 1;
          moving = true;
          break;
        case 3:
          stepper.startMove(steps_per_mm * stepperMoveAmount(stepperPos, STPP_STEPPER_DISPENSER_POS_3));
          stepperPos = stepperAbsPos(STPP_STEPPER_DISPENSER_POS_3);
          stepperStepsLeft += 1;
          moving = true;
          break;
        case 4:
          stepper.startMove(steps_per_mm * stepperMoveAmount(stepperPos, STPP_STEPPER_DISPENSER_POS_4));
          stepperPos = stepperAbsPos(STPP_STEPPER_DISPENSER_POS_4);
          stepperStepsLeft += 1;
          moving = true;
          break;
        default:
          break;
      }
      Serial.print("stepperPos after:: ");
      Serial.println(stepperPos);
      break;
    case 96:
      Serial.println("ENABLE stepper");
      stepper.setEnableActiveState(LOW);
      break;
    case 97:
      Serial.println("DISABLE stepper");
      stepper.setEnableActiveState(HIGH);
      break;
    case 98:
      Serial.println("enable stepper");
      stepper.enable();
      break;
    case 99:
      Serial.println("disable stepper");
      stepper.disable();
      break;
    case 199:
      break;
    case 200:
      Serial.println("MAKE MALIBU COLA PLEASE");
      // Create malibu cola command
      currentDrinks = drinks[argNum];
      // currentDrink = currentDrinks.drinks[0];
      // nextDrink = currentDrinks.drinks[1];
      // actions = true;
      break;
    case 210:
      webSocket.broadcastTXT("home");
      break;
    case 211:
      webSocket.broadcastTXT("redirect");
      break;
    case 212:
      webSocket.broadcastTXT("shot");
      break;
    case 213:
      webSocket.broadcastTXT("0");
      break;
    case 214:
      webSocket.broadcastTXT("1");
      break;
    default:
      break;
  }
}

// TODO: Non blocking
uint8_t rotate(Servo *s, uint8_t degrees, uint8_t start, uint8_t speed) {
  if (start < degrees) {
    for (uint8_t i = start; i < degrees; i++) {
      s->write(i);
      delay(speed);
    }
  } else {
    for (uint8_t i = start; i != degrees; i--) {
      s->write(i);
      delay(speed);
    }
  }

  return degrees;
}

int stepperAbsPos(stpp tPos) { // Get absolute position of a stpp position
  switch(tPos) {
    case STPP_STEPPER_HOME:
      return STEPPER_HOME;
    case STPP_STEPPER_DISPENSER_POS_1:
      return STEPPER_HOME + STEPPER_DISPENSER_POS_1;
    case STPP_STEPPER_DISPENSER_POS_2:
      return STEPPER_HOME + STEPPER_DISPENSER_POS_1 + STEPPER_DISPENSER_POS_2;
    case STPP_STEPPER_DISPENSER_POS_3:
      return STEPPER_HOME + STEPPER_DISPENSER_POS_1 + STEPPER_DISPENSER_POS_2 + STEPPER_DISPENSER_POS_3;
    case STPP_STEPPER_DISPENSER_POS_4:
      return STEPPER_HOME + STEPPER_DISPENSER_POS_1 + STEPPER_DISPENSER_POS_2 + STEPPER_DISPENSER_POS_3 + STEPPER_DISPENSER_POS_4;
  }

  return -1;
}

int stepperMoveAmount(int cPos, stpp tPos) { // Calculate how much movement is needed and which direction
  int aPos = stepperAbsPos(tPos);
  // if (cPos > aPos) {
    // return aPos - cPos;
  // }

  return aPos - cPos;
}

int calibrateStepper() {
  Serial.println("Start calibrating");
  stepper.startRotate(-3360); // Around 9 x 360. Safe margin for error
  int remaining = -1;

  Serial.println("Rotate until endstop gets hit");
  while (stepper.getStepsRemaining() > 0) {
    stepper.nextAction();
    if (endStopPressed) {
      remaining = stepper.stop();
    }
  }
  if (remaining <= 0) {
    Serial.println("STEPPER ERROR 1");
    return -1; // ERROR ditn't hit the endstop in time. Something is wrong
  }
  float oldRPM = stepper.getRPM();
  stepper.setRPM(20);

  // Wait for 1 second
  delay(1000);

  int remainingTemp[3] = {-1, -1, -1};
  for (uint8_t i = 0; i < 3; i++) {
    stepper.startMove(steps_per_mm * 10 * 1); // Move 1 cm

    Serial.println("Endstop got hit and going 400 steps forwards");
    while (stepper.getStepsRemaining() > 0) {
      stepper.nextAction();
    }

    // Wait for 1 second
    delay(1000);
    stepper.startMove(steps_per_mm * 10 * 2 * -1); // Move 6 cm backwards
    Serial.println("Moving 500 steps backwards until the endstop gets hit");
    while (stepper.getStepsRemaining() > 0) {
      stepper.nextAction();
      if (endStopPressed) {
        remainingTemp[i] = stepper.stop();
      }
    }

    Serial.print("remainingTemp [");
    Serial.print(i);
    Serial.print("] =");
    Serial.println(remainingTemp[i]);

    if (remainingTemp[i] <= 0 || remainingTemp[i] >= (steps_per_mm * 10 * 2) - steps_per_mm) {
      Serial.println("STEPPER ERROR 2");
      return -2; // ERROR ditn't hit the endstop in time. Something is wrong
    }
  }

  Serial.print("Remaining avarage: ");
  Serial.println((remainingTemp[1] + remainingTemp[2])/2);
  remaining = (remainingTemp[1] + remainingTemp[2])/2;

  stepper.setRPM(oldRPM);

  Serial.println("Calibration done.");
  Serial.print("Result: ");
  Serial.println(remaining);

  return remaining;
}


void onWebSocketEvent(uint8_t client_num,WStype_t type,uint8_t * payload, size_t length){
  const char *device;
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
      if(strcmp(device, "raspberry") == 0){
        int inhoud = doc["inhoud"].as<int>();
        if (doc.containsKey("noGlass")) {
          Serial.println("Glass is no longer on the platform");
          // aanwezig = false;
        }

        if (inhoud > 0 && inhoud < 100) {
          Serial.println("Move client to SHOT pagina");
          webSocket.broadcastTXT("shot");
        } else if (inhoud > 0) {
          Serial.println("Move client to KEUZEMENU pagina");
          webSocket.broadcastTXT("redirect");
        } else {
          Serial.println("Move client to HOME pagina");
          webSocket.broadcastTXT("home");
        }

        glass_ml = inhoud;
        // Serial.println("Inhoud in ml: ");
        //Doorgegeven inhoud van glas wordt in variabele gezet
        // Serial.println(inhoud);
        //Geeft seintje aan ESP dat hij door kan naar keuzeMenu pagina
        // aanwezig = true;

        // webSocket.broadcastTXT("redirect");
      }
      //Als de interface een bericht stuurt - mixdrank
      else if (strcmp(device, "web") == 0 || strcmp(device, "shot") == 0){
        String drink = doc["drink"].as<const char *>();
        String soda = doc["soda"].as<const char *>();
        if (drink == "Ba" && soda == "Co") {
          currentDrinks = drinks[0];
        } else if (drink == "Ba" && soda == "Si") {
          currentDrinks = drinks[1];
        } else if (drink == "Vo" && soda == "Co") {
          currentDrinks = drinks[2];
        } else if (drink == "Vo" && soda == "Si") {
          currentDrinks = drinks[3];
        } else if (drink == "Ma" && soda == "Co") {
          currentDrinks = drinks[4];
        } else if (drink == "Ma" && soda == "Si") {
          currentDrinks = drinks[5];
        } else if (drink == "Ma" && soda == "") {
          currentDrinks = drinks[6];
        } else if (drink == "Vo" && soda == "") {
          currentDrinks = drinks[7];
        } else if (drink == "Ba" && soda == "") {
          currentDrinks = drinks[8];
        } else if (drink == "Ro" && soda == "") {
          currentDrinks = drinks[9];
        }

        // //Kijkt of er een glas aanwezig is
        // if(aanwezig == true){
        //   //Zet drank en frisdrank in variabele en print die.
        //   Serial.print("Drink: ");
        //   drink = doc["drink"].as<const char *>();
        //   Serial.println(drink);
    
        //   Serial.print("Soda: ");
        //   soda = doc["soda"].as<const char *>();
        //   Serial.println(soda);

        //   //##################################
        //   //Hier komt de code voor het inschenken van een mixdrankje
        //   //##################################
        //   if (running) { // Er is iets fout gegaan waardoor het programma nog denk dat er nog ingeschonken wordt.
        //     return;
        //   }
        //   running = true;
        //   done = false;

        //   // AAN HET EINDE
        //   running = false;
        //   done = true;
        // }else{
        //   //Als er geen glas staat - foutmelding.
        //   //Moet in eerste instantie niet op deze pagina kunnen komen, maar als soort extra beveiliging
        //   Serial.println("Zet eerst een glas neer");
        //   webSocket.sendTXT(client_num, "{ \"type\": \"error\", \"error\": \"Glas staat niet meer op het platform\", \"redirect\": true }");
        // }

       //Als interface bericht stuurt - shot
      } else if (strcmp(device, "topi") == 0) {
        Serial.println("!!!!! DETECT GLASS !!!!!");
        webSocket.broadcastTXT("{ \"detect\": true }");
      } else if (strcmp(device, "king") == 0) {
        bool reset = doc["reset"].as<bool>();

        if (reset) {
          // Naar het begin toe en pagina naar home
          webSocket.broadcastTXT("home");
        } else {
          // Volgende shot
          if (stepperPos == stepperAbsPos(STPP_STEPPER_HOME)) {
            actions = true;
            deviceNum = 7;
            argNum = 1;
          } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_1)) {
            currentPos[0] = rotate(&servo1, SERVO_POS_MAX, currentPos[0], 20);
            delay(1000);
            currentPos[0] = rotate(&servo1, SERVO_POS_IDLE, currentPos[0], 20);
            actions = true;
            deviceNum = 7;
            argNum = 2;
          } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_2)) {
            currentPos[1] = rotate(&servo2, SERVO_POS_MAX, currentPos[1], 20);
            delay(1000);
            currentPos[1] = rotate(&servo2, SERVO_POS_IDLE, currentPos[1], 20);
            actions = true;
            deviceNum = 7;
            argNum = 3;
          } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_3)) {
            currentPos[2] = rotate(&servo3, SERVO_POS_MIN, currentPos[2], 20);
            delay(1000);
            currentPos[2] = rotate(&servo3, 180 - SERVO_POS_IDLE, currentPos[2], 20);
            actions = true;
            deviceNum = 7;
            argNum = 4;
          } else if (stepperPos == stepperAbsPos(STPP_STEPPER_DISPENSER_POS_4)) {
            Serial.println("Start moving servo");
            currentPos[3] = rotate(&servo4, SERVO_POS_MIN, currentPos[3], 20);
            delay(1000);
            currentPos[3] = rotate(&servo4, 180 - SERVO_POS_IDLE, currentPos[3], 20);
            actions = true;
            deviceNum = 7;
            argNum = 0;
            // Go back to home
            webSocket.broadcastTXT("0");
          }
        }
      }
      // }else if (strcmp(device, "shot") == 0){
      //   if(aanwezig == true){
      //     Serial.println("Dit is een shotje");

      //     //Zet drank in variabele en print die
      //     Serial.print("Soda: ");
      //     soda = doc["soda"].as<const char *>();
      //     Serial.println(soda);

      //     //##################################
      //     //Hier komt de code voor het inschenken van een shotje
      //     //##################################
      //     if (running) { // Er is iets fout gegaan waardoor het programma nog denk dat er nog ingeschonken wordt.
      //       return;
      //     }
      //     running = true;
      //     done = false;

      //     // AAN HET EINDE
      //     running = false;
      //     done = true;
      //   }else{
      //     //Als er geen glas staat - foutmelding.
      //     //Moet in eerste instantie niet op deze pagina kunnen komen, maar als soort extra beveiliging
      //     Serial.println("Zet eerst een glas neer");
      //     webSocket.sendTXT(client_num, "{ \"type\": \"error\", \"error\": \"Glas staat niet meer op het platform\", \"redirect\": true }");
      //   }
      // }
      
      break;
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

void onGamesRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [ks] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/games.html", "text/html");
}

void onGameKingRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [ks] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/ki.html", "text/html");
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

void onResetRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/reset.png", "image/png");
}

void onKingRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [img] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/img/king.jpg", "image/png");
}

// 404: als pagina niet kan vinden
void onPageNotFound(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] [404] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}