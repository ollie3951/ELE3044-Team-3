// Test sketch for Adafruit Feather ESP32-S3
// Blinks the onboard red LED and prints text to the Serial Monitor

#define LED_PIN 13 // Pin 13 is the built-in red LED on most Adafruit Feathers

void setup() {
  // Initialize the hardware serial port for USB communication
  Serial.begin(115200);
  
  // Wait up to 3 seconds for the Serial Monitor to open (essential for native USB)
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  // Configure the digital pin as an output
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("====================================");
  Serial.println("Feather ESP32-S3 Test Sketch Started");
  Serial.println("====================================");
}

void loop() {
  Serial.println("LED is ON");
  digitalWrite(LED_PIN, HIGH);  // Turn the LED on
  delay(500);                  // Wait for half a second

  Serial.println("LED is OFF");
  digitalWrite(LED_PIN, LOW);   // Turn the LED off
  delay(500);                  // Wait for half a second
}
