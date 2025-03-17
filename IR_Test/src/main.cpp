/*
    Simple app to control an IR device using an ESP32
    - Connects to WiFi
    - Provides a simple web API to send IR commands
    - Uses an onboard button to cycle through commands
    - Sends IR commands using the IR LED
    - Commands are defined in an array for easy modification
    - Uses the IRremoteESP8266 library for IR functionality
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// =============== Configuration ===============
const char* ssid = "YOUR_SSID";         // Replace with your WiFi SSID
const char* password = "YOUR_PASSWORD"; // Replace with your WiFi password
const uint16_t kIrLedPin = 4;      // IR LED connected to GPIO 4
const uint8_t buttonPin = 0;       // Onboard button

// Your IR command setup
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

IRsend irsend(kIrLedPin);
WebServer server(80);

// =============== IR Control Functions ===============
void sendIrCommand(int index) {
  if(index >= 0 && index < numCodes) {
    Serial.printf("Sending command: %s (0x%08X)\n", codeNames[index], irCodes[index]);
    irsend.sendNEC(irCodes[index], 32);
  }
}

void handleButtonPress() {
  Serial.print("Button pressed - ");
  sendIrCommand(currentIndex);
  currentIndex = (currentIndex + 1) % numCodes;
}

// =============== API Endpoints ===============
void handleApiCommand() {
  String command = server.arg("command");
  
  // Find matching command
  for(int i = 0; i < numCodes; i++) {
    if(command.equalsIgnoreCase(codeNames[i])) {
      sendIrCommand(i);
      server.send(200, "text/plain", "Command executed: " + command);
      return;
    }
  }
  
  server.send(404, "text/plain", "Command not found: " + command);
}

void handleApiList() {
  String response = "Available commands:\n";
  for(int i = 0; i < numCodes; i++) {
    response += String(i+1) + ". " + codeNames[i] + "\n";
  }
  server.send(200, "text/plain", response);
}

// =============== Setup & Loop ===============
void setup() {
  Serial.begin(115200);
  
  // Initialize IR
  irsend.begin();
  pinMode(buttonPin, INPUT_PULLUP);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure web server
  server.on("/api/command", HTTP_GET, handleApiCommand);
  server.on("/api/commands", HTTP_GET, handleApiList);
  server.begin();

  Serial.println("HTTP server started");
  Serial.println("Use:");
  Serial.println("  /api/command?command=ON");
  Serial.println("  /api/commands - to list available commands");
}

void loop() {
  server.handleClient();
  
  // Button handling
  bool buttonState = digitalRead(buttonPin);
  if(lastButtonState == HIGH && buttonState == LOW) {
    handleButtonPress();
    delay(300); // Debounce
  }
  lastButtonState = buttonState;
}
