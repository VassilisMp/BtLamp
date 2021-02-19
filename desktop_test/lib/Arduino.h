#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <chrono>
#include <iostream>
#include "pt-1.4/pt.h"

using namespace std;

class HardwareSerial {
private:
    fd_set readfds;
    queue<string> messages;

    struct timeval timeout{
            timeout.tv_sec = 0,
            timeout.tv_usec = 0
    };

    string message = "";
    int message_pointer = 0;
public:

    HardwareSerial() { // NOLINT
        FD_ZERO(&readfds); // NOLINT
    }

    int available() {
        FD_SET(STDIN_FILENO, &readfds);

        if (select(1, &readfds, nullptr, nullptr, &timeout)) {
            char message[50];
            scanf("%s", message);
            printf("Message: %s\n", message);
            string s = string(message) + "\n";
            messages.push(s);
            return 1;
        }
//        printf("...\n");
        usleep(static_cast<__useconds_t>(1 * 1000));
        return !messages.empty();
    }

    char read() {
        if (message.empty()) {
            message = messages.front();
        }
        if (message[message_pointer] != '\n')
            return message[message_pointer++];
        else {
            message.clear();
            messages.pop();
            int temp = message_pointer;
            message_pointer = 0;
            return message[temp];
        }

    }

    void print(float n) {
        printf("%.2f", n);
    }

    void print(int n) {
        printf("%d", n);
    }

    void print(unsigned long n) {
        printf("%d", n);
    }

    void println(const char* str) {
        printf("%s\n", str);
    }

    void print(const char* str) {
        printf("%s", str);
    }

    void begin(int rate) {};
};

inline long millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

#define PT_SLEEP(pt, delay) \
{ \
  do { \
    static unsigned long protothreads_sleep; \
    protothreads_sleep = millis(); \
    PT_WAIT_UNTIL(pt, millis() - protothreads_sleep > (delay)); \
  } while(false); \
}

#define LOW 0

void analogWrite(unsigned char pin, int val) {};
void digitalWrite(unsigned char pin, int val) {};

void pinMode(uint8_t pin, uint8_t mode) {};
int analogRead(uint8_t pin) { return 0; };
void randomSeed(unsigned long seed) {
    srand(time(nullptr));
}

#define INPUT 0x0
#define OUTPUT 0x1