
#define buzzerPin 17 //GPIO pin
#define pwmChannel 0 // Hardware PWM channel (0-15)

const int resolution = 8; // 8-bit resolution
const int toneFrequency = 500; // frequency of buzzer in Hz
int dutyCycle = 5; // initialise duty cycle

void setup() {
  // put your setup code here, to run once:

  // Initialize the hardware serial port for USB communication
  Serial.begin(115200);
  
  // Wait up to 3 seconds for the Serial Monitor to open (essential for native USB)
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  ledcAttach(buzzerPin, toneFrequency, resolution);

}

void loop() {
  // put your main code here, to run repeatedly:

  if(dutyCycle < 200){

    while(dutyCycle < 200){

    dutyCycle += 10;

    ledcWrite(buzzerPin, dutyCycle);
    Serial.println(dutyCycle);
    delay(200);

    }
  }

  if(dutyCycle >= 200){

    while(dutyCycle>5){

    dutyCycle -=10;

    ledcWrite(buzzerPin, dutyCycle);
    Serial.println(dutyCycle);
    delay(200);

  }

  }



  /*
  // 50% duty cycle
  ledcWrite(buzzerPin, 128); 
  delay(2000);
  
  // 10% duty cycle
  ledcWrite(buzzerPin, 25); 
  delay(2000);

  // 0% duty cycle
  ledcWrite(buzzerPin, 0);    
  delay(2000);

  analogWrite(buzzerPin, 10);
  delay(2000);
  analogWrite(buzzerPin, 0);
  delay(100);  
  */


}
