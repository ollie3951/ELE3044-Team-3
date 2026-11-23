// 15/11/2026
// O.R. C++ Code for independent implementation of the power switching subsystem

#define signalPin 0

void setup() {

  Serial.begin(115200); // establish serial communication with MCU at baud rate 115200

  // Wait up to 3 seconds for the Serial Monitor to open
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  // Configure the signal pin as an output
  pinMode(signalPin, OUTPUT);

}

void loop() {

  digitalWrite(signalPin, HIGH); // turn on green LED
  Serial.println("Green");

  delay(2000); // wait two seconds

  // trigger the relay to switch from green to red LED
  digitalWrite(signalPin, LOW);
  Serial.println("Red");

  delay(2000); // wait two seconds

}
