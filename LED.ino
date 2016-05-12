//
//    LED.ino  --  BatDevT LED user interface
//
//    Requires the dedicated services of Timer2, running in Compare Match mode
//

void SetLEDstate (byte ledPin, LEDstate state)
{
    for (int i = 0; i < Nleds; i++) {
        if (led[i].pin == ledPin) {
            noInterrupts();
            led[i].onTime = (state == Blinking) ? 25 : -1;
            led[i].counter = 0;
            interrupts();
            break;
        }
    }
    if (state == On)
        digitalWriteFast(ledPin, HIGH);
    else if (state == Off)
        digitalWriteFast(ledPin, LOW);

    return;

}


void InitLEDs (void)
{
    for (int i = 0; i < Nleds; i++)
        pinMode(led[i].pin, OUTPUT);

    InitTimer2();

}


void InitTimer2 (void)    // Set up Timer2 to generate Compare-Match interrupts
{
    TCCR2B = 0;               // Stop the timer (etc.)
    TCCR2A = 0;               // Normal operation for pins 3 & 11 (etc.)
    TIMSK2 = 0;               // Disable all timer interrupts.

    bitSet(TCCR2A, WGM21);    // WGM22:20 = 2 sets up CTC counter mode.

    TCNT2 = 0;                // Clear the timer counter.
    OCR2A = 156;              // Establish the TOP value for CTC mode.
    bitSet(TIMSK2, OCIE2A);   // Enable interrupts on matches to OCR2A.

    TCCR2B = 7;               // CS22:0 = 7 sets the prescaler dividing by
                              // 1024, which starts the timer running again.
}


ISR(TIMER2_COMPA_vect)    // Interrupt handler, runs approx every 10 milliseconds
{
    for (int i = 0; i < Nleds; i++) {
        if (led[i].onTime == -1)
            continue;
        if (++led[i].counter >= led[i].onTime) {
            led[i].counter = 0;
            togglePin(led[i].pin);
        }
    }
}


static void togglePin (byte pin)      // Invert voltage on LED pin
{
    switch (pin) {
      case RedLED:
        digitalToggleFast(RedLED);
        break;
      case GreenLED:
        digitalToggleFast(GreenLED);
        break;
      case BlueLED:
        digitalToggleFast(BlueLED);
        break;

    }
}
