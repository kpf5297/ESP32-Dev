#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

const uint16_t kIrLedPin = 4;      // IR LED connected to GPIO 4
const uint8_t buttonPin = 0;       // ESP32 onboard button or external button

IRsend irsend(kIrLedPin);

uint32_t irCodes[] = {
  0x807FA15E,  // ON
  0x807FC936,  // Dim
  0x807F718E,  // Dark Blue
  0x807F21DE   // OFF
};

const char* codeNames[] = {"ON", "Dim", "Dark Blue", "OFF"};
const int numCodes = sizeof(irCodes) / sizeof(irCodes[0]);
int currentIndex = 0;
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  irsend.begin();
  pinMode(buttonPin, INPUT_PULLUP); // Use pull-up resistor for button
  Serial.println("IR Transmitter Ready. Press button to cycle commands.");
}

void loop() {
  bool buttonState = digitalRead(buttonPin);

  // Detect button press (falling edge)
  if (lastButtonState == HIGH && buttonState == LOW) {
    Serial.print("Sending IR Command: ");
    Serial.println(codeNames[currentIndex]);
    
    irsend.sendNEC(irCodes[currentIndex], 32); // Send 32-bit NEC code
    
    currentIndex = (currentIndex + 1) % numCodes; // Cycle through codes

    delay(300); // Simple debounce delay
  }

  lastButtonState = buttonState;
}
