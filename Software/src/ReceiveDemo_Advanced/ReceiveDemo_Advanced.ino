#define RX_PIN 14

void setup() {
  Serial.begin(115200);
  pinMode(RX_PIN, INPUT);
  delay(2000);
  Serial.println("=== Ready ===");
}

void loop() {
  static int last = -1;
  static unsigned long lastChange = 0;
  static int changeCount = 0;
  
  int val = digitalRead(RX_PIN);
  if(val != last) {
    changeCount++;
    last = val;
    lastChange = millis();
  }
  
  // Alle 1000ms Statistik ausgeben
  static unsigned long lastReport = 0;
  if(millis() - lastReport > 1000) {
    Serial.print("Wechsel/Sekunde: ");
    Serial.println(changeCount);
    changeCount = 0;
    lastReport = millis();
  }
}
/*

#define TX_PIN 15  // D8
#define RX_PIN 14  // D5

unsigned int timings[400];
int timingsLen = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  digitalWrite(TX_PIN, LOW);
  delay(2000);
  Serial.println("=== Ready ===");
  Serial.println("Taste druecken zum Aufzeichnen, p = Wiedergeben");
}

void recordSignal() {
  timingsLen = 0;

  // Warte auf Sync-Puls (LOW > 5000µs = echtes Signal)
  unsigned long syncPulse = 0;
  while(syncPulse < 5000) {
    syncPulse = pulseIn(RX_PIN, LOW, 100000);
    yield();
  }

  Serial.print("Sync erkannt: ");
  Serial.print(syncPulse);
  Serial.println("µs - Aufzeichne...");

  for(int i = 0; i < 200; i++) {
    unsigned int h = pulseIn(RX_PIN, HIGH, 10000);
    unsigned int l = pulseIn(RX_PIN, LOW,  10000);
    yield();
    if(h == 0 || l == 0) {
      timingsLen = i * 2;
      break;
    }
    timings[i*2]   = h;
    timings[i*2+1] = l;
  }

  Serial.print("Aufgezeichnet: ");
  Serial.print(timingsLen / 2);
  Serial.println(" Pulse");
}

void playSignal(int repeat = 5) {
  // Sync zuerst senden
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(500);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(7000);
  
  for(int r = 0; r < repeat; r++) {
    for(int i = 0; i < timingsLen; i += 2) {
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(timings[i]);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(timings[i+1]);
    }
    // Sync zwischen Wiederholungen
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(7000);
    yield();
  }
}

void loop() {
  if(Serial.available()) {
    char c = Serial.read();
    if(c == 'r') {
      Serial.println("Warte auf Signal...");
      recordSignal();
    }
    if(c == 'p') {
      if(timingsLen == 0) {
        Serial.println("Nichts aufgezeichnet!");
      } else {
        Serial.println("Wiedergabe...");
        playSignal();
        Serial.println("Fertig");
      }
    }
  }
  yield();
}
/*
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== Ready ===");
  mySwitch.enableReceive(14); //D5
  mySwitch.setReceiveTolerance(60);
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
*/
/*
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);
  mySwitch.enableReceive(14); // D5
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
    Serial.print(mySwitch.getReceivedProtocol());
    Serial.print(" Raw: ");
    unsigned int* raw = mySwitch.getReceivedRawdata();
    for (int i = 0; i < mySwitch.getReceivedBitlength() * 2 + 2; i++) {
      Serial.print(raw[i]);
      Serial.print(" ");
    }
    Serial.println();
    mySwitch.resetAvailable();
  }
}
*/