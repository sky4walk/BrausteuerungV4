/*
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

#define TX_PIN 15

void setup() {
  Serial.begin(115200);
  mySwitch.enableTransmit(TX_PIN);
  mySwitch.setProtocol(13);
  mySwitch.setPulseLength(360);
  mySwitch.setRepeatTransmit(1);
}

void loop() {
  // Obere 24 Bit von 462420691
  Serial.println("Test ON obere 24 Bit: 1808046");
  mySwitch.send(1808046, 24);
  mySwitch.send(1808046, 24);
  delay(4000);

  // Obere 24 Bit von 149854355  
  Serial.println("Test OFF obere 24 Bit: 585316");
  mySwitch.send(585316, 24);
  mySwitch.send(585316, 24);
  delay(4000);

  // Obere 24 Bit von 1350987155
  Serial.println("Test ON2 obere 24 Bit: 5277075");
  mySwitch.send(5277075, 24);
  mySwitch.send(5277075, 24);
  delay(4000);

  // Obere 24 Bit von 3561711827
  Serial.println("Test OFF2 obere 24 Bit: 13913718");
  mySwitch.send(13913718, 24);
  mySwitch.send(13913718, 24);
  delay(4000);

  delay(10000);
}
*/

#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

#define TX_PIN 15  // D8

const unsigned long CODE_A_ON_1  = 462420691;
const unsigned long CODE_A_ON_2  = 1350987155;
const unsigned long CODE_A_OFF_1 = 149854355;
const unsigned long CODE_A_OFF_2 = 3561711827;

void sendCode(unsigned long c1, unsigned long c2, int cycles = 4) {
  for (int i = 0; i < cycles; i++) {
    mySwitch.send(c1, 24);
    mySwitch.send(c1, 24);
    delay(385);
//    mySwitch.send(c2, 24);
//    mySwitch.send(c2, 24);
//    delay(385);
  }
}

void sendA_ON()  { sendCode(CODE_A_ON_1,  CODE_A_ON_2); }
void sendA_OFF() { sendCode(CODE_A_OFF_1, CODE_A_OFF_1); }

void setup() {
  Serial.begin(115200);
  mySwitch.enableTransmit(15); //D8
  mySwitch.setProtocol(13);
  mySwitch.setPulseLength(360);
  mySwitch.setRepeatTransmit(1);
}

void loop() {
  Serial.println("A OFF");
  mySwitch.send(462420691, 24);
  mySwitch.send(462420691, 24);
//  mySwitch.send(149854355, 24);
//  mySwitch.send(149854355, 24);
  delay(5000);
}

/*
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();
#define RC_SWITCH 15 


// Clevenwils Codes
const long CL1_ON  = 910432256;
const long CL1_OFF = 1044649984;
const long CL2_ON  = 373561344;
const long CL2_OFF = 507779072;
const long CL3_ON  = 239343616;
const long CL3_OFF = 21239808;
const long CL4_ON  = 641996800;
const long CL4_OFF = 776214528;
*/
/*
// BEARWARE
const int AON       = 13488751;
const int AOFF      = 13488750;
const int BITS      = 24;
const int PROTOCOL  = 1;
const int PULSE_LEN = 311;
const int REAPEAT   = 15;


void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== Ready ===");

  mySwitch.enableTransmit(RC_SWITCH);
  mySwitch.setProtocol(PROTOCOL);
  mySwitch.setPulseLength(PULSE_LEN);
  mySwitch.setRepeatTransmit(REAPEAT);
}

void loop() {
  Serial.println("Steckdose 1 ON");
  mySwitch.send(AON, BITS);
  delay(3000);

  Serial.println("Steckdose 1 OFF");
  mySwitch.send(AOFF, BITS);
  delay(3000);

}
*/
// -----------------

/*
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

#define TX_PIN 15  // D8

// A ON
const unsigned long CODE_A_ON_1 = 462420691;
const unsigned long CODE_A_ON_2 = 1350987155;

// A OFF
const unsigned long CODE_A_OFF_1 = 149854355;
const unsigned long CODE_A_OFF_2 = 3561711827;

void sendCode(unsigned long c1, unsigned long c2, int bits = 32, int cycles = 8) {
  for (int i = 0; i < cycles; i++) {
    mySwitch.send(c1, bits);
    mySwitch.send(c1, bits);
    yield();
    mySwitch.send(c2, bits);
    mySwitch.send(c2, bits);
    yield();
  }
}

void sendA_ON()  { sendCode(CODE_A_ON_1,  CODE_A_ON_2,24);  }
void sendA_OFF() { sendCode(CODE_A_OFF_1, CODE_A_OFF_2,24); }

void setup() {
  Serial.begin(115200);
  mySwitch.enableTransmit(TX_PIN);
  mySwitch.setProtocol(14);  // neues Protokoll!
  mySwitch.setPulseLength(500);
  mySwitch.setRepeatTransmit(1);
}

void loop() {
  Serial.println("Test 1: Protocol 13, 24 Bit");
  mySwitch.setProtocol(13);
  mySwitch.setPulseLength(360);
  sendCode(CODE_A_ON_1, CODE_A_ON_2, 24);
  delay(3000);
  sendCode(CODE_A_OFF_1, CODE_A_OFF_2, 24);
  delay(3000);

  Serial.println("Test 2: Protocol 13, 32 Bit");
  mySwitch.setProtocol(13);
  mySwitch.setPulseLength(360);
  sendCode(CODE_A_ON_1, CODE_A_ON_2, 32);
  delay(3000);
  sendCode(CODE_A_OFF_1, CODE_A_OFF_2, 32);
  delay(3000);

  Serial.println("Test 3: Protocol 14, 24 Bit");
  mySwitch.setProtocol(14);
  mySwitch.setPulseLength(500);
  sendCode(CODE_A_ON_1, CODE_A_ON_2, 24);
  delay(3000);
  sendCode(CODE_A_OFF_1, CODE_A_OFF_2, 24);
  delay(3000);

  Serial.println("Test 4: Protocol 14, 32 Bit");
  mySwitch.setProtocol(14);
  mySwitch.setPulseLength(500);
  sendCode(CODE_A_ON_1, CODE_A_ON_2, 32);
  delay(3000);
  sendCode(CODE_A_OFF_1, CODE_A_OFF_2, 32);
  delay(3000);

  delay(10000);
}
*/