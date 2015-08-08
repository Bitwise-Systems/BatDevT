//---------------------------------------------------------------------------------------
//    DriverTimer.ino  --  Simple timer-event-driven task scheduler
//
//    In what follows, the T1 prescaler is set to divide by 256, which implies (given
//    the 16MHz CPU clock) a timer resolution of 16 microseconds (256 / 16M). Since
//    T1 uses a 16-bit counter, we can go for up to 1.05 seconds ((2^16 - 1) * 16 us)
//    between interrupts.
//
//    Version 2: Added support for virtual timers.
//
//---------------------------------------------------------------------------------------


void (*callbackFunction)() = NULL;     // Callback function pointer


void StartTimer (OneShotTimerID id, unsigned long count)   // Start one-shot timer
{
    if (id < NumOneShot) {
        cli();
        oneShot[id] = count;
        sei();
    }
}


void ResyncTimer (FreeRunningTimerID id)    // Resynchronize free-running timer
{
    if (id < NumFreeRunning) {
        cli();
        freeRunning[id].count = freeRunning[id].reload;
        freeRunning[id].state = Running;
        sei();
    }
}


boolean IsRunning (OneShotTimerID id)    // Query one-shot timer
{
    boolean status;

    if (id < NumOneShot) {
        cli();
        status = (oneShot[id] == 0) ? false : true;  // False => no longer running
        sei();
        return status;
    }
    return true;              // Nonexistent timers never time out
}


boolean HasExpired (FreeRunningTimerID id)    // Query free-running timer
{
    boolean state;

    if (id < NumFreeRunning) {
        cli();
        state = freeRunning[id].state;
        freeRunning[id].state = Running;
        sei();
        return state;          // True => yes, timer has expired
    }
    return false;

}


void InitTimerTask (unsigned interval, void (*callback)())
{
    callbackFunction = callback;

    TCCR1B = 0;      // Stop the timer, et al.
    TCCR1A = 0;      // Normal operation for pins 9 & 10.
    TCCR1C = 0;      // Disable forced compare services.
    TIMSK1 = 0;      // Disable all T1 timer interrupts.

    bitSet(TCCR1B, WGM12);        // WGM13:10 = 4 (CTC counter mode).
    bitSet(TIMSK1, OCIE1A);       // Enable interrupts for matches to OCR1A.

    TCNT1 = 0;                                   // Clear the timer counter.
    OCR1A = (62 * interval) + (interval >> 1);   // Establish the TOP value for CTC mode.

    for (int i = 0; i < NumFreeRunning; i++) {          // Initialize data structure
        freeRunning[i].count = freeRunning[i].reload;
        freeRunning[i].state = Running;
    }

    bitSet(TCCR1B, CS12);         // CS12:0 = 4, which sets the prescaler divisor to
                                  // 256, and thereby starts the timer running again.
}


ISR(TIMER1_COMPA_vect)      // TIMER1 compare-match interrupt service routine:
{
    unsigned long t;        // Non-volatile working variable prevents double fetch...

    for (int i = 0; i < NumOneShot; i++) {
        if ((t = oneShot[i]) != 0)
            oneShot[i] = --t;
    }
    for (int i = 0; i < NumFreeRunning; i++) {
        if (freeRunning[i].count > 1)
            freeRunning[i].count--;
        else {
            freeRunning[i].count = freeRunning[i].reload;
            freeRunning[i].state = Expired;
        }
    }
    sei();
    if (callbackFunction != NULL)
        (*callbackFunction)();

}
