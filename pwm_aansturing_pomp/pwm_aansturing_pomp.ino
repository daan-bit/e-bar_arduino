const int pin = D5;

void setup() {
  pinMode(pin, OUTPUT);

}

void loop() {
  analogWrite(pin, 200);
  delay(20);
  analogWrite(pin, 0);
  delay(20);

}
