#include <Arduino.h>
// #include <SoftwareSerial.h>
#include <stdlib.h>
#include "protothreads.h"
// #include <vector>
// #include <time.h>
// #include <Thread.h>
// #include <ThreadController.h>
#include <math.h>
#include <SoftwareSerial.h>

#define random_byte() (byte)(random(256))

#define LOGGING
inline void log(const char[]) __attribute__((always_inline));

void log(const char c[]) {
#ifdef LOGGING
    Serial.println(c);
#endif
}

void log(const byte *red, const byte *green, const byte *blue, const byte *alpha, const char message[] = nullptr, const char *prefix = nullptr, const char *suffix = nullptr) {
#ifdef LOGGING
    if (message) Serial.print(message);
    if (prefix) Serial.print(*prefix);
    char s[35];
    sprintf(s, " {%3d, %3d, %3d, %3d}", *red, *green, *blue, *alpha);
    log(s);
    if (suffix) Serial.print(*suffix);
#endif
}

// sign function
#define sgn(x) (x > 0) - (x < 0)

// signal width
double A = 1.0;
// period in milliseconds
unsigned long T = 0;
// human perception is as fast as 80ms
#define MIN_T 80UL
#define MAX_T 30000UL
#define M_2PI 6.283185307179586232
// All trigonometric functions listed have period 2π
//double trigonometric_period = M_2PI;

// limit value between [0, 1]
#define limit_0_1(x) (x>1 ? 1 : x<0 ? 0 : x)
// periodic functions
#define f(t) (M_2PI*t)/T
float sine_t(double t) { return abs(sin(f(t))); }
#define enable_sine_t() periodicFun = &sine_t;
float cos_t(double t) { return abs(cos(f(t))); }
#define enable_cos_t() periodicFun = &cos_t;
// #define cas_t(t) (sine_t(t) + cosine_t(t) * 2 // multiply to set max width to 1 // same with sine, just with different frequency
float tangent_t(double t) { return limit_0_1(tan(f(t))); }
#define enable_tangent_t() periodicFun = &tangent_t;
// default
float square_t(double t) { return limit_0_1(sgn(sin(f(t)))); }
#define enable_square_t() periodicFun = &square_t;
float triangle_t(double t) { return abs((2 * A / M_PI) * asin(sin(f(t)))); }
#define enable_triangle_t() periodicFun = &triangle_t;

#define PUMP_PIN 9
#define RED_PIN 5
#define GREEN_PIN 3
#define BLUE_PIN 6
#define RX_PIN 12
#define TX_PIN 13
#define END_CHAR '\n'

SoftwareSerial MyBlue(RX_PIN, TX_PIN); // RX | TX
// Bluetooth codes
#define CHANGE_COLOR 'c'
#define CHANGE_POWER_INTERVAL 'i'
#define ENABLE_RANDOM_COLOR 'R'
#define DISABLE_RANDOM_COLOR 'r'
#define SUBMIT_COLOR_SEQUENCE 's'
#define REMOVE_COLOR_SEQUENCE 'S'
#define ENABLE_LIGHT 'L'
#define DISABLE_LIGHT 'l'
#define ENABLE_PUMP 'P'
#define DISABLE_PUMP 'p'
#define ENABLE_SINE '1'
#define ENABLE_COSINE '2'
#define ENABLE_TANGENT '3'
#define ENABLE_SQUARE '4'
#define ENABLE_TRIANGLE '5'

//#define PTS_SIZE 2
// typedef int(*ThreadFPointer)(pt *pt);

// inline static std::vector<pt> pts{2}
// pt pts[PTS_SIZE];
// int (*ptsFuns[PTS_SIZE])(pt *pt);
// ThreadFPointer ptFuns[PTS_SIZE];

// function to copy an array
void copyArray(byte *arr, byte *copy, int size)
{
	// loop to iterate through array
	for (int i = 0; i < size; ++i) {
		copy[i] = arr[i];
	}
}

pt intervalPt;

byte pump_level = 255;
byte red_level = 255;
byte green_level = 255;
byte blue_level = 255;
byte alpha_level = 255;
float alpha_coef = 1.0;
//boolean dimming = false;
boolean random_color_mode = false;
byte random_color_speed = 1;
// three speed levels MIN_T, T, T/2
unsigned long random_color_T() { return random_color_speed == 0 ? MIN_T : random_color_speed == 1 ? T : T/2; }
//boolean color_seq_mode = false;
boolean light_state = false;
boolean pump_state = false;
// read buffer
#define BUFFER_SIZE 100
byte buffer[BUFFER_SIZE];
//byte index = 0;
size_t bytesRead = 0;
//byte end_index = 0;
byte color_seq_array[100];
byte color_seq_multitude;

typedef int (*thread_ptr)();
typedef float (*periodic_fun)(double);
// TODO future use
/*
typedef struct Color {
   byte red;
   byte green;
   byte blue;
   byte alpha;
} Color;
*/
// thread_ptr threads[PTS_SIZE];
thread_ptr thread;
//thread_ptr color_seq_thread = nullptr;
periodic_fun periodicFun = &square_t;

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
void removeColorSequence();
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
int periodic_light();
//int color_sequence();

// convert components of a color int
byte alpha(unsigned long color) {
    // unsigned right shift operation
    return color >> 24;
}
byte red(unsigned long color) {
    return (color >> 16) & 0xFF;
}
byte green(unsigned long color) {
    return (color >> 8) & 0xFF;
}
byte blue(unsigned long color) {
    return color & 0xFF;
}
// packing an array of 4 bytes to an long, big endian else little endian, clean code
long toLong(const byte d[], boolean bigEndian) {
    long value; 
    if (bigEndian) {
        value  =  (long)d[0] << 24;
        value += (long)d[1] << 16;
        value += (long)d[2] << 8;
        value += (long)d[3];
    } else {
        /*return ((bytes[3] & 0xFF) << 24) | 
            ((bytes[2] & 0xFF) << 16) | 
            ((bytes[1] & 0xFF) << 8 ) | 
            ((bytes[0] & 0xFF) << 0 );*/
        value  =  (long)d[3] << 24;
        value += (long)d[2] << 16;
        value += (long)d[1] << 8;
        value += (long)d[0];
    }
    return value;
}

//inline void runThreads() __attribute__((always_inline));


// TODO write bluetooth setup code, SoftwareSerial...
// reads from bluetooth serial
void btRead() {
    // https://www.arduino.cc/reference/en/language/functions/communication/serial/readbytes/
    // Serial.readBytes(buffer, length)
    if(MyBlue.available() > 0) {
        //buffer[index] = MyBlue.read();
        // if reached end character, execute cases by message
        bytesRead = MyBlue.readBytesUntil(END_CHAR, buffer, BUFFER_SIZE);
        /* do stuff with message */
        switch (buffer[0]) {
            case CHANGE_COLOR:
                if (random_color_mode) disableRandomColor();
                changeColor(buffer + 1);
                break;
            case CHANGE_POWER_INTERVAL:
                changePowerInterval(buffer + 1);
                break;
            case ENABLE_RANDOM_COLOR:
                enableRandomColor();
                random_color_speed = buffer[1];
                break;
            case DISABLE_RANDOM_COLOR:
                disableRandomColor();
                break;
            case SUBMIT_COLOR_SEQUENCE:
                submitColorSequence(buffer + 1);
                break;
            case REMOVE_COLOR_SEQUENCE:
                removeColorSequence();
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
            case ENABLE_SINE:
                enable_sine_t();
                break;
            case ENABLE_COSINE:
                enable_cos_t();
                break;
            case ENABLE_TANGENT:
                enable_tangent_t();
                break;
            case ENABLE_SQUARE:
                enable_square_t();
                break;
            case ENABLE_TRIANGLE:
                enable_triangle_t();
                break;
            default:
                break;
        }
    }
}

void changeColor(const byte *red, const byte *green, const byte *blue, const byte *alpha = nullptr) {
    log(red, green, blue, alpha, "changeColor");
    if (alpha) {
        alpha_level = *alpha;
        // alpha coefficient
        alpha_coef = alpha_level/255.0;
    }
    red_level = round(*red*alpha_coef);
    green_level = round(*green*alpha_coef);
    blue_level = round(*blue*alpha_coef);
    if (!thread && light_state)
        lightOn();  
}

void changeColor(byte message[]) {
    byte *alpha = message[3] == END_CHAR ? nullptr : &message[3];
    changeColor(&message[0], &message[1], &message[2], alpha);
}

void changePowerInterval(const byte message[]) {
    // T = * (unsigned long *) message;
    T = toLong(message, true);
    #ifdef LOGGING
    //    int size = sizeof message / sizeof message[0];
        Serial.print("byte array size: ");
        Serial.println(bytesRead-1);
    //    Serial.println(size);
    //    Serial.println((char*)message);
        Serial.print("changePowerInterval: ");
        Serial.print(T);
        Serial.println(" ms");
    #endif
    // Serial.println(power_interval_ms);
    if (T > 0 && !thread) {
        thread = &periodic_light;
    } else if (T == 0) {
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
    log("submitColorSequence");
    // use index-1 because is on the newline(last) char
    // count quadraples of (r,g,b,a)
    color_seq_multitude = (bytesRead-1)/4;
    byte colorSeq[bytesRead-1];
    copyArray(message, color_seq_array, bytesRead-1);
    #ifdef LOGGING
        Serial.print('['); 
        char s[5];
        for (int i = 0; i < bytesRead-1; i+=4) {
            sprintf(s, "{%3d, %3d, %3d, %3d}, ", color_seq_array[i], color_seq_array[i+1], color_seq_array[i+2], color_seq_array[i+3]);
            Serial.print(s); 
        }
        Serial.print("] ");
        Serial.print(", multitude ");
        Serial.print(int(color_seq_multitude)); 
        Serial.println();
    #endif
    // deactivate random color if enabled, to use color sequence
    if (random_color_mode) {
        random_color_mode = false;
        // TODO must send update to android if deactivated
    }
}

void removeColorSequence() {
    log("removeColorSequence");
    color_seq_multitude = 0;
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
int periodic_light() {
//    static auto coeff = M_2PI / T;
    static float dimm_coeff;
    static unsigned long t;
    static unsigned long last_c_change = 0;
    static int seq_index = 0;
    PT_BEGIN(&intervalPt);
    t = millis();
    if (random_color_mode && t - last_c_change >= random_color_T()) {
        red_level = random_byte() * alpha_coef;
        green_level = random_byte() * alpha_coef;
        blue_level = random_byte() * alpha_coef;
//        changeColor(&red, &green, &blue);
        last_c_change = t;
    }
    // if color sequence array is not null, change color
    if (color_seq_multitude>0 && t - last_c_change >= random_color_T()) {
        // each color is a qudraple, (r, g, b, alpha)
        // I don 't use alpha, so I can change color without dimming 
        red_level = color_seq_array[seq_index]; //* alpha_coef;
        green_level = color_seq_array[seq_index+1]; //* alpha_coef;
        blue_level = color_seq_array[seq_index+2]; //* alpha_coef;
        log(&red_level, &green_level, &blue_level, &alpha_level);
        // add 4 to go to the next color
        seq_index += 4;
        if (seq_index/color_seq_multitude == 4) seq_index = 0; 
//        changeColor(&red, &green, &blue);
        last_c_change = t;
    }
    
    // math
//    log("dimming");
    // dimm_coeff = abs(sqrt(1-sqrt(1-(t*coeff)))); for circle
    dimm_coeff = (*periodicFun)(t);
    // Serial.println(dimm_coeff);
//    printf("%f\n", dimm_coeff);
    if (light_state) dimLight(dimm_coeff);
    if (pump_state) dimPump(dimm_coeff);
    PT_SLEEP(&intervalPt, MIN_T);
    PT_END(&intervalPt);
}

/*void runThreads() {
    for (int i = 0; i < PTS_SIZE; i++) {
        threads[i]();
    }
}*/

void setup() {
    PT_INIT(&intervalPt);
    Serial.begin(9600);
    MyBlue.begin(9600);
    // setup pins
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    randomSeed(analogRead(0));
    enableRandomColor();
    enableLight();
    unsigned long interv = 0;
    changePowerInterval((byte*)&interv);
}

void loop() {
    btRead();
    //runThreads();
    if (thread) (*thread)();
    //if (color_seq_thread) (*color_seq_thread)();
}