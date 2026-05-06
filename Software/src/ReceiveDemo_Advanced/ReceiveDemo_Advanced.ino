#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== Ready ===");
  mySwitch.enableReceive(D5);
  mySwitch.setReceiveTolerance(80);
}

void loop() {
  if (mySwitch.available()) {
    Serial.print("Value: ");
    Serial.print(mySwitch.getReceivedValue());
    Serial.print(" Bits: ");
    Serial.print(mySwitch.getReceivedBitlength());
    Serial.print(" Delay: ");
    Serial.print(mySwitch.getReceivedDelay());
    Serial.print(" Protocol: ");
    Serial.println(mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
  }
}