#define RXp2 12
#define TXp2 13
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXp2, TXp2);
}
void loop() { //Choose Serial1 or Serial2 as required
  while (Serial2.available()) {
    Serial.print(char(Serial2.read()));
  }
}
