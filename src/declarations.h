#include <Arduino.h>
// #include <SoftwareSerial.h>
#include <math.h>
#include <stdlib.h>
#include "protothreads.h"
#include <vector>
// #include <time.h>
// #include <Thread.h>
// #include <ThreadController.h>

#define PUMP_PIN 9
#define RED_PIN 5
#define GREEN_PIN 3
#define BLUE_PIN 6
#define RX_PIN 0
#define TX_PIN 1
#define END_CHAR '\n'


#define round(x)     (byte)((x)+0.5)
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

#define PTS_SIZE 2

class Engine {
    // inline static std::vector<pt> pts{2}
    inline static pt pts[PTS_SIZE];
    int (*fun[PTS_SIZE])(pt *pt);

    byte pump_level = 0;
    byte red_level = 0;
    byte green_level = 0;
    byte blue_level = 0;
    byte alpha_level = 0;
    float alpha_coef = 0.0;
    unsigned long power_interval_ms = 0;
    static const unsigned long MAX_INTERVAL = 5000;
    boolean random_color_mode = false;
    boolean color_seq_mode = false;
    boolean light_state = false;
    boolean pump_state = false;
    // read buffer
    byte buffer[100];
    byte index = 0;
    byte end_index = 0;

public:
    Engine() {
        // initialize protothreads
        for (int i = 0; i < count; i++) {
            PT_INIT(&pts[i]);
        }    
    }
    // callback for ioThread
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

    void changeColor(byte *red, byte *green, byte *blue, byte *alpha) {
        red_level = *red;
        green_level = *green;
        blue_level = *blue;
        if (alpha != NULL) {
            alpha_level = *alpha;
            // alpha coefficient
            alpha_coef = alpha_level/255.0;
        }
        // turn off random color
        disableRandomColor();
        analogWrite(RED_PIN, round(red_level*alpha_coef));
        analogWrite(GREEN_PIN, round(green_level*alpha_coef));
        analogWrite(BLUE_PIN, round(blue_level*alpha_coef));
        if (pump_state) {
            analogWrite(PUMP_PIN, round(255*alpha_coef));
        }
    }

    void changeColor(byte message[]) {
        byte *alpha = message[3] == END_CHAR ? nullptr : &message[3];
        changeColor(&message[0], &message[1], &message[2], alpha);
    }

    void changePowerInterval(byte message[]) {
        power_interval_ms = * (unsigned long *) message;
        if (power_interval_ms > 0) {

        } else if (power_interval_ms == 0) {
        }
    }

    void enableRandomColor() {
        random_color_mode = true;
    }

    void disableRandomColor() {
        random_color_mode = false;
    }

    void submitColorSequence(byte message[]) {

    }

    void lightOn() {
        light_state = true;
        if (power_interval_ms > 0 ) {
            if (!showThread.enabled) {
                controll.add(&showThread);
            }
        }
        analogWrite(RED_PIN, round(red_level*alpha_coef));
        analogWrite(GREEN_PIN, round(green_level*alpha_coef));
        analogWrite(BLUE_PIN, round(blue_level*alpha_coef));
        if (pump_state) {
            analogWrite(PUMP_PIN, round(255*alpha_coef));
        }
    }

    void lightOff() {
        light_state = false;
        if (showThread.enabled)
        {
            controll.remove(&showThread);  
        }
        digitalWrite(RED_PIN, LOW);
        digitalWrite(GREEN_PIN, LOW);
        digitalWrite(BLUE_PIN, LOW);
    }

    void pumpOn() {
        pump_state = true;
        analogWrite(PUMP_PIN, round(255*alpha_coef));
    }

    void pumpOff() {
        pump_state = false;
        digitalWrite(PUMP_PIN, LOW);
    }

    void turnOff() {
        digitalWrite(RED_PIN, LOW);
        digitalWrite(GREEN_PIN, LOW);
        digitalWrite(BLUE_PIN, LOW);
        digitalWrite(PUMP_PIN, LOW);
    }

    void turnOn() {

    }

    // threads
    static int protothread1(struct pt *pt) {
        /* A protothread function must begin with PT_BEGIN() which takes a
           pointer to a struct pt. */
        PT_BEGIN(pt);
        /* We loop forever here. */
        while(1) {
            /* Wait until the other protothread has set its flag. */
            PT_WAIT_UNTIL(pt, protothread2_flag != 0);
//                        printf("Protothread 1 running\n");

            /* We then reset the other protothread's flag, and set our own
               flag so that the other protothread can run. */
            protothread2_flag = 0;
            protothread1_flag = 1;

            /* And we loop. */
        }

        /* All protothread functions must end with PT_END() which takes a
           pointer to a struct pt. */
        PT_END(pt);
    }
};
