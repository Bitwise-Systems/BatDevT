//---------------------------------------------------------------------------------------
//    DriverTLynx.ino -- Low-level drivers for Lineage Power's Pico TLynx DC-DC power
//                       module (Sparkfun BOB-09370) and Linear Technologies LTC4352
//                       ideal diode.
//
//    Version 1:  First version with INA219 support moved to a separate file.
//                Equivalent to 'Drivers.ino' version 7.
//
//---------------------------------------------------------------------------------------


void InitTLynx (void)
{
    pinMode(PowerON, OUTPUT);
    PowerOff();
    InitPot();

}


static int potLevel = 512;        // Potentiometer powers up mid-scale

void InitPot (void)
{
    pinMode(PotDirection, OUTPUT);
    pinMode(PotToggle, OUTPUT);
    digitalWriteFast(PotDirection, HIGH);  // Instruct counter to increment
    digitalWriteFast(PotToggle, HIGH);

    for (int n = 0; n < 1024; n++) {       // Move wiper to 'H' end of the ladder
       digitalWriteFast(PotToggle, LOW);   // ...giving maximum resistance and
       digitalWriteFast(PotToggle, HIGH);  // ...thereby, minimum voltage.
    }
    potLevel = 1023;                       // Reflect situation in shadow variable

}


int NudgeVoltage (int request)
{
    int newLevel = constrain((potLevel - request), 0, 1023);
    int actual = potLevel - newLevel;
    digitalWriteFast(PotDirection, (actual < 0) ? HIGH : LOW);

    for (int n = abs(actual); n > 0; n--) {
       digitalWriteFast(PotToggle, LOW);
       digitalWriteFast(PotToggle, HIGH);
    }
    potLevel = newLevel;
    return actual;
}


void SetPotLevel (int value)
{
    int newPotLevel = constrain(value, 0, 1023);

    NudgeVoltage(potLevel - newPotLevel);
    potLevel = newPotLevel;

}


int GetPotLevel (void)
{
    return potLevel;
}


float SetVoltage (float setV)
{
    setV = constrain(setV, SetVLow, SetVHigh);

    SetPotLevel(MapVoltageToLevel(setV));
    return setV;

}


static int MapVoltageToLevel (float v)
{                                           // Use '219 chip address as a "system ID"
    int sysID = Get219Address();            // ...to determine proper mapping function.

    if (sysID == 0x44)          // Hutch
        return (int) (144782.58398414083 - 255329.6260662532 * (v - sqrt(0.32749571094960045 + (-1.1372942571138163 + v) * v)));    // 03-28-15

    else if (sysID == 0x41)     // Mike
        return (int) (247730.23973477256 - 440578.9548398173 * (v - sqrt(0.319841240475064 + (-1.1265521875173699 + v) * v)));      // 11-22-15

    else                        // Unknown
        return 1023;

}


void PowerOn (void)
{
     digitalWriteFast(PowerON, HIGH);     // MOSFET driver inverts for a negative logic TLynx
     delay(20);                           // Let it stabilize.
}


void PowerOff (void)
{
     digitalWriteFast(PowerON, LOW);
}


boolean PowerGoodQ (void)    // Return TLynx 'PGD' pin status
{
    return ((analogRead(PowerGood) == 0) ? 0 : 1);
}


boolean StatusQ (void)       // Read ideal diode status pin
{
    return digitalReadFast(DiodeStatus);

}
