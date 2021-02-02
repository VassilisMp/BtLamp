#include <Arduino.h>

#define PUMP_PIN 9
#define RED_PIN 5
#define GREEN_PIN 3
#define BLUE_PIN 6

void setup() {
  // setup pins
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
}

int val = 0;

void loop() {
  for(int i=0; i<255; i++){
    analogWrite(RED_PIN, i);
    delay(10);
  }
  delay(100);
  analogWrite(RED_PIN, 0);
  for(int i=0; i<255; i++){
    analogWrite(GREEN_PIN, i);
    delay(10);
  }
  delay(100);
  analogWrite(GREEN_PIN, 0);
  for(int i=0; i<255; i++){
    analogWrite(BLUE_PIN, i);
    delay(10);
  }
  delay(100);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);
  delay(100);
  analogWrite(RED_PIN, 254);
  analogWrite(GREEN_PIN, 254);
  analogWrite(BLUE_PIN, 254);
  delay(2000);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);
//  analogWrite(PUMP_PIN, 0);
//  delay(100);
//  for(int i=0; i<255; i++){
//    analogWrite(RED_PIN, i);
//    delay(10);
//  }
//  analogWrite(RED_PIN, 0);
//  delay(100);
//  analogWrite(PUMP_PIN, 254);
//  analogWrite(RED_PIN, 254);
//  analogWrite(GREEN_PIN, 254);
//  analogWrite(BLUE_PIN, 254);
//  delay(100);
}