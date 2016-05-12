//
//    LED.h  --  BatDevT LED user interface
//

typedef enum { Off, On, Blinking } LEDstate;

struct LEDstruct {
    const byte pin;    // Arduino pin driving the LED.
    int onTime;        // LED on-time (& off-time) in 10's of milliseconds.
    int counter;       // Used internally to decide when to flash.
};

volatile struct LEDstruct led[] = {
    {RedLED,   -1, 0},
    {GreenLED, -1, 0},
    {BlueLED,  -1, 0},
};

#define Nleds (sizeof led / sizeof(struct LEDstruct))