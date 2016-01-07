//---------------------------------------------------------------------------------------
//      Loads.ino  --  Drivers for discharger board (and other) resistive loads
//---------------------------------------------------------------------------------------

void InitLoads (void)
{
    pinMode(HeavyLoadGate, OUTPUT);
    pinMode(MediumLoadGate, OUTPUT);
    pinMode(LightLoadGate, OUTPUT);

    AllLoadsOff();    // Includes setting BusLoad to hi-Z & no pull-up on BatDetect

}


void HeavyOn (void)
{
    digitalWriteFast(HeavyLoadGate, HIGH);
}


void HeavyOff (void)
{
    digitalWriteFast(HeavyLoadGate, LOW);
}


void MediumOn (void)
{
    digitalWriteFast(MediumLoadGate, HIGH);
}


void MediumOff (void)
{
    digitalWriteFast(MediumLoadGate, LOW);
}


void LightOn (void)
{
    digitalWriteFast(LightLoadGate, HIGH);
}


void LightOff (void)
{
    digitalWriteFast(LightLoadGate, LOW);
}


//    The INA219 is impressing a "ghost" voltage onto the positive bus (V- pin).
//    Switching in the BusLoad resistor reduces this voltage to about a quarter
//    of the unloaded value. On Mike's system, the non-load voltage is 1.076 V;
//    loaded it's 0.289 volts (12/14/15). On Hutch's: 1.052 and 0.278 V (12/17/15).

void LoadBus (void)
{
    pinMode(BusLoad, OUTPUT);
    digitalWriteFast(BusLoad, LOW);     // Ground bus through a 4.3K resistor
}


void UnLoadBus (void)
{
    pinMode(BusLoad, INPUT);        // HI-Z to stop loading the bus
}


void Impress (void)         // Impress +5V on the battery bus
{
    pinMode(BatDetect, INPUT_PULLUP);    // Connect Vcc to bus thru 20-50K pullup

}


void RemoveImpress (void)
{
    pinMode(BatDetect, INPUT);

}


void AllLoadsOff (void)
{
    HeavyOff();
    MediumOff();
    LightOff();
    UnLoadBus();
    RemoveImpress();

}


void UnknownLoad (void)    // Used by 'LoadCmd' in handling unknown argument
{
    Printf("Unknown load\n");
}


void LoadStatus (void)    // Portability caution: 'BusLoad' port/pin hard-coded below
{
    Printf("Load    Heavy  Medium  Light  Bus\n");
    Printf("Status:  %s    %s     %s   %s\n",
        digitalReadFast(HeavyLoadGate) ? "ON " : "OFF",
        digitalReadFast(MediumLoadGate) ? "ON " : "OFF",
        digitalReadFast(LightLoadGate) ? "ON " : "OFF",
        bitRead(DDRD, 7) ? "ON " : "OFF"
    );

}


void LoadByCapacity (void)
{
    if (capacity < 1000) {                           // @1.2V, 120mA
        LightOn();
    }
    if (1000 <= capacity && capacity < 1600) {       // @1.2V, 255mA
        MediumOn();
    }
    if (1600 <= capacity && capacity < 2100) {       // @1.2V, 375mA
        LightOn();
        MediumOn();
    }
    if (2100 <= capacity && capacity < 3500) {       // @1.2V, 600mA
        HeavyOn();
    }
    if (3500 <= capacity && capacity < 5000) {       // @1.2V, 855mA
        MediumOn();
        HeavyOn();
    }
    if (5000 <= capacity) {                          // @1.2V, 975mA
        LightOn();
        MediumOn();
        HeavyOn();
    }

}


// void LoadCheck (void)
// {
//     float shuntMA;
//
//     Printf("Ideal diode MOSFET is %s\n", (StatusQ() == 0) ? "ON" : "OFF");
//
//     Monitor(&shuntMA, NULL);
//     Printf("%1.4f mA\n", shuntMA);
// }
