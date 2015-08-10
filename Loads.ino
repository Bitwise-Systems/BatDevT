//
//      Loads.ino  --  Drivers for discharger board resistive loads
//

void InitLoads (void)
{
    pinMode(HeavyLoadGate, OUTPUT);
    pinMode(MediumLoadGate, OUTPUT);
    pinMode(LightLoadGate, OUTPUT);
    pinMode(BusLoad, INPUT);             // HI-Z means no affect on bus voltage
    pinMode(BatDetect, INPUT_PULLUP);    // reads high until inserted battery pulls it low
}


void HeavyOn (void)
{
    digitalWrite(HeavyLoadGate, HIGH);
}


void HeavyOff (void)
{
    digitalWrite(HeavyLoadGate, LOW);
}


void MediumOn (void)
{
    digitalWrite(MediumLoadGate, HIGH);
}


void MediumOff (void)
{
    digitalWrite(MediumLoadGate, LOW);
}


void LightOn (void)
{
    digitalWrite(LightLoadGate, HIGH);
}


void LightOff (void)
{
    digitalWrite(LightLoadGate, LOW);
}

void LoadBus (void)                      // INA219 is 'leaking' ~1.3V onto busbars with no battery
{                                        // .. load bus to reduce extra voltage to ~ 200mV
    pinMode(BusLoad, OUTPUT);
    digitalWrite(BusLoad, LOW);             // ground positive batt bus (via ~ 1K load resistor)
}


void UnLoadBus (void)
{
    pinMode(BusLoad, INPUT);                   // make it HI-Z to stop loading the bus
}

//------------------- temporary aux functions ------------

void LoadCheck (void)
{
    float shuntMA;

    Printf("Ideal diode MOSFET is %s\n", (StatusQ() == 0) ? "ON" : "OFF");

    Monitor(&shuntMA, NULL);
    Printf("%1.4f mA\n", shuntMA);
}

