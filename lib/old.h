#include <Arduino.h>
// #include <SoftwareSerial.h>
#include <math.h>
#include <stdlib.h>
// #include <time.h>
#include <Thread.h>
#include <ThreadController.h>

#define PUMP_PIN 9
#define RED_PIN 5
#define GREEN_PIN 3
#define BLUE_PIN 6
#define RX_PIN 0
#define TX_PIN 1
#define END_CHAR '\n'


#define random_byte() (byte)(random(256))

// Bluetooth codes
#define CHANGE_COLOR 'c'
#define CHANGE_POWER_INTERVAL 'i'
#define ENABLE_RANDOM_COLOR 'R'
#define DISABLE_RANDOM_COLOR 'r'
#define SUBMIT_COLOR_SEQUENCE 's'
#define LIGHT_ON 'L'
#define LIGHT_OFF 'l'
#define PUMP_ON 'P'
#define PUMP_OFF 'p'

// variables
// SoftwareSerial MyBlue(RX_PIN, TX_PIN); // RX | TX 
byte pump_level = 0;
byte red_level = 0;
byte green_level = 0;
byte blue_level = 0;
byte alpha_level = 0;
float alpha_coef = 0.0;
unsigned long power_interval_ms = 0;
const unsigned long MAX_INTERVAL = 5000;
boolean random_color_mode = false;
boolean color_seq_mode = false;
boolean light_state = false;
boolean pump_state = false;
// read buffer
byte buffer[100];
byte index = 0;
byte end_index = 0;

// functions
inline void changeColor(byte[]) __attribute__((always_inline));
inline void changeColor(byte, byte, byte, byte) __attribute__((always_inline));
void changePowerInterval(byte[]);
void changePowerInterval(unsigned long);
inline void enableRandomColor() __attribute__((always_inline));
inline void disableRandomColor() __attribute__((always_inline));
void submitColorSequence(byte[]);
void lightOn();
void lightOff();
inline void pumpOn() __attribute__((always_inline));
inline void pumpOff() __attribute__((always_inline));


// ThreadController that will controll all threads
ThreadController controll = ThreadController();

Thread ioThread = Thread();
Thread showThread = Thread();
Thread showThreadHelper = Thread();

// callback for ioThread
void btRead()
{
	// https://www.arduino.cc/reference/en/language/functions/communication/serial/readbytes/
  // Serial.readBytes(buffer, length)
  if(Serial.available() > 0) {
    buffer[index] = Serial.read();
    // if reached end character, execute cases by message
    if (buffer[index] == END_CHAR)
    {
      /* do stuff with message */
      switch (buffer[0])
      {
      case CHANGE_COLOR:
        changeColor(buffer + 1);
        break;
      case CHANGE_POWER_INTERVAL:
        changePowerInterval(buffer + 1);
        break;
      case ENABLE_RANDOM_COLOR:
        enableRandomColor();
        break;
      case DISABLE_RANDOM_COLOR:
        disableRandomColor();
        break;
      case SUBMIT_COLOR_SEQUENCE:
        submitColorSequence(buffer + 1);
        break;
      case LIGHT_ON:
        lightOn();
        break;
      case LIGHT_OFF:
        lightOff();
        break;
      case PUMP_ON:
        pumpOn();
        break;
      case PUMP_OFF:
        pumpOff();
        break;
      default:
        break;
      }
      index = 0;
    } else {
      index++;
    }
  }
}

// callback for showThread
void show()
{
  if (random_color_mode)
  {
    changeColor(random_byte(), random_byte(), random_byte(), NULL);
  } else if (color_seq_mode)
  {
    /* code */
  }
  showThreadHelper.reset(10);
}

void light_off()
{
  Serial.println("showThreadHelper: light_off");
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  // controll.remove(&showThreadHelper);
}

void setup() {
  Serial.begin(9600);
  // setup pins
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  // Configure threads
  // ioThread conf
	ioThread.onRun(btRead);
	ioThread.setInterval(0);
  // showThread conf
  showThread.onRun(show);
  showThread.setInterval(0);
  showThread.enabled = false;
  // helperThread conf
  showThreadHelper.onRun(light_off);
  showThreadHelper.setInterval(10);
  showThreadHelper.enabled = false;
  // conf controller
  controll.add(&ioThread);
  // controll.add(&showThread);
  // initialize random generator
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
  init();
}

void init() 
{
  enableRandomColor();
  lightOn();
  // pumpOff();
  changePowerInterval((unsigned long) 1000);
}

void loop() {
  // run ThreadController
	// this will check every thread inside ThreadController,
	// if it should run. If yes, he will run it;
	controll.run();
}

void changeColor(byte red, byte green, byte blue, byte alpha)
{
  red_level = red;
  green_level = green;
  blue_level = blue;
  if (alpha != NULL)
  {
    alpha_level = alpha;
    // alpha coefficient
    alpha_coef = alpha_level/255.0;
  }
  // turn off random color
  // disableRandomColor();
  analogWrite(RED_PIN, round(red_level*alpha_coef));
  analogWrite(GREEN_PIN, round(green_level*alpha_coef));
  analogWrite(BLUE_PIN, round(blue_level*alpha_coef));
  if (pump_state)
  {
    analogWrite(PUMP_PIN, round(255*alpha_coef));
  }
}

void changeColor(byte message[]) {
  changeColor(message[0], message[1], message[2], message[3]);
}

void changePowerInterval(byte message[]) 
{
}

void changePowerInterval(unsigned long interval) 
{
  power_interval_ms = interval;
  if (power_interval_ms > 0 && !showThread.enabled)
  {
    controll.add(&showThread);
    // controll.add(&showThreadHelper);
  } else if (power_interval_ms == 0 && showThread.enabled)
  {
    controll.remove(&showThread);
    controll.remove(&showThreadHelper);
  }
  showThread.setInterval(power_interval_ms);
}

void enableRandomColor()
{
  random_color_mode = true;
}

void disableRandomColor()
{
  random_color_mode = false;
}

void submitColorSequence(byte message[])
{

}

void lightOn()
{
  light_state = true;
  if (power_interval_ms > 0 )
  {
    if (!showThread.enabled)
    {
      controll.add(&showThread);
    }
  }
  analogWrite(RED_PIN, round(red_level*alpha_coef));
  analogWrite(GREEN_PIN, round(green_level*alpha_coef));
  analogWrite(BLUE_PIN, round(blue_level*alpha_coef));
  if (pump_state)
  {
    analogWrite(PUMP_PIN, round(255*alpha_coef));
  }
}

void lightOff() 
{
  light_state = false;
  if (showThread.enabled)
  {
    controll.remove(&showThread);  
  }
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}

void pumpOn() 
{
  pump_state = true;
  analogWrite(PUMP_PIN, round(255*alpha_coef));
}

void pumpOff()
{
  pump_state = false;
  digitalWrite(PUMP_PIN, LOW);
}