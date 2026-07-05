#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>


// GSM Module Pins
SoftwareSerial mySerial(8, 7); // RX = Pin 10, TX = Pin 11 
//connect cable according to pin number or else edit the number in the code 

// GPS Module
const int RXPin = 2;
const int TXPin = 3;
const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// MPU6050 Sensor
const int zPin = A1;
// Array of phone numbers for emergency contacts
const char* phoneNumbers[] = {
  "+911122334455" // change your number - inserted in gsm
};
const int numPhoneNumbers = sizeof(phoneNumbers) / sizeof(phoneNumbers[0]);

// Timing & Flags
bool eventTriggered = false;
unsigned long lastEventTime = 0;
const int debounceDelay = 10000;

void setup() {
  Serial.begin(9600);
  mySerial.begin(115200);
  ss.begin(GPSBaud);

  Serial.println("Initializing GSM Module...");
  sendATCommand("AT");
  delay(1000);
  sendATCommand("AT+CSCS=\"GSM\"");
  delay(1000);
}

void loop() {
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  int zRaw = analogRead(zPin);

 
  // float yVolt = yRaw * (3.3 / 1023.0);

  // // Convert voltage to acceleration (in g)
  // // ADXL335 outputs ~1.65V at 0g, and ~330mV per g
  
  // float yAcc = (yVolt - 1.65) / 0.33;

  Serial.print("Z: "); Serial.print(zRaw); Serial.println(" g, ");
  if (zRaw>250) {

    Serial.println("Alert Triggered: Sending SMS and Making Call...");
    eventTriggered = true;
    lastEventTime = millis();

    sendGPSData();
    delay(5000);

    for (int i = 0; i < numPhoneNumbers; i++) {
      makeCall(phoneNumbers[i]);
      delay(5000);
    }
  }

  if (eventTriggered && (millis() - lastEventTime >= debounceDelay)) {
    eventTriggered = false;
    Serial.println("Resetting alert trigger...");
  }

  delay(1000);
}

void sendGPSData() {
  String smsMessage;
  Serial.println("Waiting for GPS fix...");
  unsigned long startTime = millis();
  while (!gps.location.isValid() && millis() - startTime < 10000) {
    while (ss.available()) {
      gps.encode(ss.read());
    }
  }

  if (gps.location.isValid()) {
    smsMessage = "Emergency Location: https://www.google.com/maps/search/?api=1&query=";
    smsMessage += String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    Serial.println("GPS Fix Acquired");
  } else {
    smsMessage = "Emergency Alert! GPS location not available.";
  }

  for (int i = 0; i < numPhoneNumbers; i++) {
    sendSMS(phoneNumbers[i], smsMessage);
    delay(5000);
  }
}

void sendSMS(const char* number, const String& text) {
  Serial.println("Sending SMS...");
  sendATCommand("AT+CMGF=1");
  delay(1000);
  String command = "AT+CMGS=\"";
  command += number;
  command += "\"";
  sendATCommand(command.c_str());
  delay(1000);
  mySerial.println(text);
  delay(500);
  mySerial.write(26);
  delay(5000);
  Serial.println("SMS Sent!");
}

void makeCall(const char* number) {
  Serial.print("Calling "); Serial.println(number);
  String command = "ATD";
  command += number;
  command += ";";
  sendATCommand(command.c_str());
  delay(20000);
  hangUpCall();
}

void hangUpCall() {
  Serial.println("Hanging up the call...");
  sendATCommand("AT+CHUP");
}

void sendATCommand(const char* cmd) {
  mySerial.println(cmd);
  delay(1000);
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
}