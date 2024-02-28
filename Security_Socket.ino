#include "EmonLib.h"
#include <SoftwareSerial.h>

EnergyMonitor emon1;

int Red = 5;
int Green = 6;
int Blue = 7;
int Sms = 0;

int CT_pin = 1;
int Sense = 3;

int PowerOnCounter = 0;
int PowerOffCounter = 0;

volatile boolean LedState_1 = HIGH;
volatile boolean lastLedState_1 = HIGH;

volatile boolean LedState_2 = HIGH;
volatile boolean lastLedState_2 = HIGH;

volatile boolean LedState_3 = HIGH;
volatile boolean lastLedState_3 = HIGH;

enum State {
  IDLE,
  POWER_ON,
  SAFE_UNPLUG,
  ALERT_POWER_ON,
  POWER_OFF,
  STOP,
  ALERT_POWER_OFF,
  RESET_SYSTEM
};

State currentState = IDLE;
SoftwareSerial mySerial(13, 12);

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Blue, OUTPUT);
  pinMode(Sense, INPUT_PULLUP);

  digitalWrite(Green, LOW);
  digitalWrite(Red, LOW);
  digitalWrite(Blue, LOW);

  mySerial.println("AT+CMGF=1");  // set SMS text mode
  delay(100);
  mySerial.println("AT+CNMI=1,2,0,0,0");  // set SIM800L to notify when new SMS is received
  delay(100);

  emon1.current(CT_pin, 32.59);
}

void loop() {
  switch (currentState) {
    case IDLE:
      idle();
      break;
    case POWER_ON:
      power_on();
      break;
    case SAFE_UNPLUG:
      safe_unplug();
      break;
    case POWER_OFF:
      power_off();
      break;
    case ALERT_POWER_ON:
      alert_power_On();
      break;
    case ALERT_POWER_OFF:
      alert_power_Off();
      break;
    case RESET_SYSTEM:
      reset_system();
      break;
    case STOP:
      stop();
      break;
  }
}
void idle() {
  if (digitalRead(3) == LOW) {
    currentState = POWER_ON;
    // Serial.println("POWER ON");
  } else {
    currentState = POWER_OFF;
    //Serial.println("POWER OFF");
  }
}
void power_on() {
  idle();
  safe_unplug();
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  float ApparentPower = Irms * 230.0;
  float Value = Irms;
  //when Power is On and Reset is not  done
  if ((ApparentPower > 148 && ApparentPower < 370) && (Value > 0.67 && Value < 1.62) && PowerOnCounter != 7 ) {
    Serial.println("Power On: Load Connected");
    digitalWrite(Green, HIGH);
    digitalWrite(Red, LOW);
    digitalWrite(Blue, LOW);
    Sms = 0;
    //safe_unplug();
    delay(100);
  } else if (ApparentPower > 370 && Value > 1.62 && PowerOnCounter != 7) {
    Serial.println("Power On: Load Connected");
    digitalWrite(Green, HIGH);
    digitalWrite(Red, LOW);
    digitalWrite(Blue, LOW);
    //PowerOnCounter = 3;
    //safe_unplug();
    Sms = 0;
    delay(100);
  }
  if (ApparentPower < 148 && Value < 0.67 && PowerOnCounter != 7) {
    Serial.println("Power On: Load Not Connected Power Turned ON");
    digitalWrite(Green, LOW);
    digitalWrite(Red, HIGH);
    digitalWrite(Blue, LOW);
    currentState = ALERT_POWER_ON;
  }
}
void stop() {
  if (mySerial.available()) {                        // check if there is a message available
    String message = mySerial.readString();          // read the message
    Serial.println("Received message: " + message);  // print the message to the serial monitor
    if (message.indexOf("START") != -1) {            // if the message contains "ON"
      digitalWrite(Green, LOW);
      digitalWrite(Red, LOW);
      digitalWrite(Blue, HIGH);
      delay(1000);
      PowerOnCounter = 4;
      currentState = IDLE;
    }
  }
}
void safe_unplug() {
  if (mySerial.available()) {                        // check if there is a message available
    String message = mySerial.readString();          // read the message
    Serial.println("Received message: " + message);  // print the message to the serial monitor
    if (message.indexOf("STOP") != -1) {             // if the message contains "ON"
      digitalWrite(Green, LOW);
      digitalWrite(Red, LOW);
      digitalWrite(Blue, HIGH);
      PowerOnCounter = 7;
      delay(1000);
      currentState = STOP;
    }
  }
}
void alert_power_On() {
  if (Sms == 0) {
    mySerial.println("AT");  //Once the handshake test is successful, it will back to OK
    updateSerial();
    mySerial.println("AT+CMGF=1");  // Configuring TEXT mode
    updateSerial();
    mySerial.println("AT+CMGS=\"+254113061790\"");  //change ZZ with country code and xxxxxxxxxxx with phone number to sms
    updateSerial();
    mySerial.print("EMERGENCY ALERT THEFT DETECTED: POWER ON");  //text content
    updateSerial();
    mySerial.write(26);
    Serial.println("SENDING ALERT MESSAGE: POWER ON");
    Sms = 1;
    currentState = RESET_SYSTEM;
  }
}
void reset_system() {
  if (Sms == 1) {
    if (mySerial.available()) {                        // check if there is a message available
      String message = mySerial.readString();          // read the message
      Serial.println("Received message: " + message);  // print the message to the serial monitor
      if (message.indexOf("RESET") != -1) {            // if the message contains "ON"
        Serial.println("SYSTEM IS RESET SUCCESSFULLY");
        currentState = IDLE;
      }
    }
  }
}
void power_off() {
  idle();
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  float ApparentPower = Irms * 230.0;
  float Value = Irms;
  if (ApparentPower < 65.0) {
    digitalWrite(Green, HIGH);
    digitalWrite(Red, LOW);
    digitalWrite(Blue, LOW);
    Serial.println("Power Off: Load Connected");
    // Serial.print("\t");
    // Serial.print("Apparent Power: ");
    // Serial.print(ApparentPower);
    // Serial.print("\t");
    // Serial.print("Value: ");
    // Serial.println(Value);
  } else if (ApparentPower > 65.0) {
    digitalWrite(Green, LOW);
    digitalWrite(Red, HIGH);
    digitalWrite(Blue, LOW);
    Serial.println("Power Off: Load Disconnected");
    // Serial.print("\t");
    // Serial.print("Apparent Power: ");
    // Serial.print(ApparentPower);
    // Serial.print("\t");
    // Serial.print("Value: ");
    // Serial.println(Value);
  }
}
void alert_power_Off() {
  if (PowerOnCounter == 0) {
    // mySerial.println("AT");  //Once the handshake test is successful, it will back to OK
    // updateSerial();
    // mySerial.println("AT+CMGF=1");  // Configuring TEXT mode
    // updateSerial();
    // mySerial.println("AT+CMGS=\"+254113061790\"");  //change ZZ with country code and xxxxxxxxxxx with phone number to sms
    // updateSerial();
    // mySerial.print("EMERGENCY ALERT THEFT DETECTED: POWER OFF");  //text content
    // updateSerial();
    // mySerial.write(26);
    Serial.println("SENDING ALERT MESSAGE: POWER OFF");
    //PowerOnCounter = 1;
    currentState = RESET_SYSTEM;
  }
}
void updateSerial() {
  delay(500);
  while (Serial.available()) {
    mySerial.write(Serial.read());  //Forward what Serial received to Software Serial Port
  }
  while (mySerial.available()) {
    Serial.write(mySerial.read());  //Forward what Software Serial received to Serial Port
  }
}