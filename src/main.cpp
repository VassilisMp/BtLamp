#include <Arduino.h>
// #include <SoftwareSerial.h>
#include <stdlib.h>
#include "protothreads.h"
// #include <vector>
// #include <time.h>
// #include <Thread.h>
// #include <ThreadController.h>

#define random_byte() (byte)(random(256))

#define LOGGING
inline void log(const char[]) __attribute__((always_inline));

void log(const char c[]) {
#ifdef LOGGING
    Serial.println(c);
#endif
}

#define PUMP_PIN 9
#define RED_PIN 5
#define GREEN_PIN 3
#define BLUE_PIN 6
#define RX_PIN 0
#define TX_PIN 1
#define END_CHAR '\n'

// Bluetooth codes
#define CHANGE_COLOR 'c'
#define CHANGE_POWER_INTERVAL 'i'
#define ENABLE_RANDOM_COLOR 'R'
#define DISABLE_RANDOM_COLOR 'r'
#define SUBMIT_COLOR_SEQUENCE 's'
#define ENABLE_LIGHT 'L'
#define DISABLE_LIGHT 'l'
#define ENABLE_PUMP 'P'
#define DISABLE_PUMP 'p'
#define ENABLE_DIMMING 'D'
#define DISABLE_DIMMING 'd'

#define PTS_SIZE 2
#define MAX_INTERVAL 1500UL

// typedef int(*ThreadFPointer)(pt *pt);

// inline static std::vector<pt> pts{2}
// pt pts[PTS_SIZE];
// int (*ptsFuns[PTS_SIZE])(pt *pt);
// ThreadFPointer ptFuns[PTS_SIZE];

pt intervalPt;

byte pump_level = 255;
byte red_level = 255;
byte green_level = 255;
byte blue_level = 255;
byte alpha_level = 255;
float alpha_coef = 1.0;
int power_interval_ms = 0;
boolean dimming = false;
boolean random_color_mode = false;
boolean color_seq_mode = false;
boolean light_state = false;
boolean pump_state = false;
// read buffer
byte buffer[100];
byte index = 0;
byte end_index = 0;

typedef int (*thread_ptr)();
// thread_ptr threads[PTS_SIZE];
thread_ptr thread;

// light functions
/*
* turns on the light with the last values
*/
inline void lightOn() __attribute__((always_inline));
/*
* turns off the light
*/
inline void lightOff() __attribute__((always_inline));
// enables light by last state
inline void enableLight() __attribute__((always_inline));
// disables light
inline void disableLight() __attribute__((always_inline));
// enables random color mode
inline void enableRandomColor() __attribute__((always_inline));
// disables random color mode
inline void disableRandomColor() __attribute__((always_inline));
// change color
inline void changeColor(byte message[]) __attribute__((always_inline));
/*
* changes color and disables random color if enabled, configures pump based on alpha
*/
inline void changeColor(const byte *red, const byte *green, const byte *blue, const byte *alpha) __attribute__((always_inline));
void changePowerInterval(const byte[]);
void submitColorSequence(byte[]);
void dimLight(float coeff);
// pump functions
// turns on the pump according to the alpha coefficient
inline void pumpOn() __attribute__((always_inline));
// turns off the pump
inline void pumpOff() __attribute__((always_inline));
// enables the pump by last state
inline void enablePump() __attribute__((always_inline));
// disables the pump
inline void disablePump() __attribute__((always_inline));
// dimm pump
void dimPump(float coeff);
// enable dimming
inline void enableDimming() __attribute__((always_inline));
// disable dimming
inline void disableDimming() __attribute__((always_inline));

int intervalSwitch();
int dimmingTriangle();

//inline void runThreads() __attribute__((always_inline));


// reads from bluetooth serial
void btRead() {
    // https://www.arduino.cc/reference/en/language/functions/communication/serial/readbytes/
    // Serial.readBytes(buffer, length)
    if(Serial.available() > 0) {
        buffer[index] = Serial.read();
        // if reached end character, execute cases by message
        if (buffer[index] == END_CHAR) {
            /* do stuff with message */
            switch (buffer[0]) {
                case CHANGE_COLOR:
                    disableRandomColor();
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
                case ENABLE_DIMMING:
                    enableDimming();
                    break;
                case DISABLE_DIMMING:
                    disableDimming();
                    break;
                case SUBMIT_COLOR_SEQUENCE:
                    submitColorSequence(buffer + 1);
                    break;
                case ENABLE_LIGHT:
                    enableLight();
                    break;
                case DISABLE_LIGHT:
                    disableLight();
                    break;
                case ENABLE_PUMP:
                    enablePump();
                    break;
                case DISABLE_PUMP:
                    disablePump();
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

void changeColor(const byte *red, const byte *green, const byte *blue, const byte *alpha) {
    log("changeColor");
    if (alpha != nullptr) {
        alpha_level = *alpha;
        // alpha coefficient
        alpha_coef = alpha_level/255.0;
    }
    red_level = round(*red*alpha_coef);
    green_level = round(*green*alpha_coef);
    blue_level = round(*blue*alpha_coef);
}

void changeColor(byte message[]) {
    byte *alpha = message[3] == END_CHAR ? nullptr : &message[3];
    changeColor(&message[0], &message[1], &message[2], alpha);
}

void changePowerInterval(const byte message[]) {
    power_interval_ms = * (int *) message;
#ifdef LOGGING
    Serial.print("changePowerInterval: ");
    Serial.print(power_interval_ms);
    Serial.println(" ms");
#endif
    // Serial.println(power_interval_ms);
    if (power_interval_ms > 0) {
        if (dimming) thread = &dimmingTriangle;
        else thread = &intervalSwitch;
    } else if (power_interval_ms == 0) {
        thread = nullptr;
    }
}

void enableRandomColor() {
    log("enableRandomColor");
    random_color_mode = true;
}

void disableRandomColor() {
    log("disableRandomColor");
    random_color_mode = false;
}

void submitColorSequence(byte message[]) {
    // TODO
}

void lightOn() {
    log("lightOn");
    analogWrite(RED_PIN, red_level);
    analogWrite(GREEN_PIN, green_level);
    analogWrite(BLUE_PIN, blue_level);
}

void lightOff() {
    log("lightOff");
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
}

void enableLight() {
    log("enableLight");
    light_state = true;
    lightOn();
}

void disableLight() {
    log("disableLight");
    light_state = false;
    lightOff();
}

void pumpOn() {
    log("pumpOn");
    analogWrite(PUMP_PIN, round(255*alpha_coef));
}

void pumpOff() {
    log("pumpOff");
    digitalWrite(PUMP_PIN, LOW);
}

void enablePump() {
    log("enablePump");
    pump_state = true;
    pumpOn();
}

void disablePump() {
    log("disablePump");
    pump_state = false;
    pumpOff();
}

void dimLight(float coeff) {
    analogWrite(RED_PIN, round(red_level*coeff));
    analogWrite(GREEN_PIN, round(green_level*coeff));
    analogWrite(BLUE_PIN, round(blue_level*coeff));
}

void dimPump(float coeff) {
    analogWrite(RED_PIN, round(pump_level*coeff));
}

// threads
int intervalSwitch() {
    PT_BEGIN(&intervalPt);
    log("intervalSwitch");
    for(;;) {
        if (random_color_mode) {
            byte red = random_byte();
            byte green = random_byte();
            byte blue = random_byte();
            changeColor(&red, &green, &blue, nullptr);
        } else if (color_seq_mode) {
            /* code */
        }
        if (light_state) lightOn();
        if (pump_state) pumpOn();
        PT_SLEEP(&intervalPt, power_interval_ms/2);
        if (light_state) lightOff();
        if (pump_state) pumpOff();
        PT_SLEEP(&intervalPt, power_interval_ms/2);
    }
    PT_END(&intervalPt);
}

int dimmingTriangle() {
    static int half_t = power_interval_ms/2;
    static auto coeff = static_cast<float>(100.0 / half_t);
    static float dimm_coeff;
    static int i;
    PT_BEGIN(&intervalPt);
    log("dimmingTriangle");
    if (random_color_mode) {
        auto red = random_byte();
        auto green = random_byte();
        auto blue = random_byte();
        changeColor(&red, &green, &blue, nullptr);
    }
    // math
    log("dimming up");
    for (i = 0; i <= power_interval_ms/2; i++) {
        dimm_coeff = i*coeff;
        // Serial.println(dimm_coeff);
//        printf("%f\n", dimm_coeff);
        if (light_state) dimLight(dimm_coeff);
        if (pump_state) dimPump(dimm_coeff);
        PT_SLEEP(&intervalPt, 1);
    }
    log("dimming down");
    for (i = power_interval_ms/2; i >= 0; i--) {
        dimm_coeff = i*coeff;
//        printf("%f\n", dimm_coeff);
        if (light_state) dimLight(dimm_coeff);
        if (pump_state) dimPump(dimm_coeff);
        PT_SLEEP(&intervalPt, 1);
    }
    PT_END(&intervalPt);
}

void enableDimming() {
    log("enableDimming");
    dimming = true;
    if (thread == &intervalSwitch)
        thread = &dimmingTriangle;
}

void disableDimming() {
    log("disableDimming");
    dimming = false;
    if (thread == &dimmingTriangle)
        thread = &intervalSwitch;
}

/*void runThreads() {
    for (int i = 0; i < PTS_SIZE; i++) {
        threads[i]();
    }
}*/

void setup() {
    PT_INIT(&intervalPt);
    Serial.begin(9600);
    // setup pins
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    randomSeed(analogRead(0));
//    enableDimming();
    enableLight();
    int interv = 200;
    changePowerInterval((byte*)&interv);
}

void loop() {
    btRead();
    // runThreads();
    if (thread) {
        (*thread)();
    }
}