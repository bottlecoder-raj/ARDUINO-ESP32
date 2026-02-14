#include <Servo.h>
int pos=90;

Servo myservo;
bool S1 ,S2;
void setup() {
  // put your setup code here, to run once:
  myservo.attach(9); 
  pinMode(10,INPUT);
  pinMode(11,INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  S1=digitalRead(10);
  S2=digitalRead(11);
  Serial.print(S1);
  Serial.print("\t");
  Serial.println(S2);
  if(S1 && !S2){
    pos++;
    myservo.write(pos);
  }
  else if(!S1 && S2){
    pos--;
    myservo.write(pos);
  }
  else if (!S1 && !S2) {
    myservo.write(pos);
  }
  delay(20);
}
