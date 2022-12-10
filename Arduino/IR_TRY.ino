int IR_SENSOR = 2;

void setup() {
  pinMode(IR_SENSOR, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
  int STATUS_SENSOR = digitalRead(IR_SENSOR);
  Serial.begin(9600);
  Serial.println(STATUS_SENSOR);
  if (STATUS_SENSOR == 0){
     digitalWrite(LED_BUILTIN, HIGH);
     delay(10);}

  else{
    digitalWrite(LED_BUILTIN, LOW);
    delay(10);
      }
}
