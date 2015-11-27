//=======================================================================================
//    MidLevel.ino -- Repository for routines that perform a significant amount
//                    of work in connecting the command handlers to the low-level
//                    drivers.
//=======================================================================================



exitStatus BailOutQ (void)
{
    float busV, batteryTemp, ambientTemp;

    Monitor(NULL, &busV);
    GetTemperatures(&batteryTemp, &ambientTemp);

    exitStatus bailRC = Success;       // success means no bail-outs tripped

    if (batteryTemp > MaxBatTemp)   bailRC = PanicTemp;
    if (busV > MaxBatVolt)          bailRC = PanicVoltage;
    if (!PowerGoodQ())              bailRC = PBad;
    if (StatusQ() == 1)             bailRC = IdealDiodeStatus;    // trips if used during off pulse
    if (Serial.available() > 0)     bailRC = ConsoleInterrupt;
    if (bailRC != Success)
        Printf("(* Bail! %1.3f V, %1.2f deg *)\n", busV, batteryTemp);

    return bailRC;

}


exitStatus BatteryPresentQ (float busV)
{
    return Success;         // <<< TEMPORARY: checking effect of removing battery detection circuitry >>>

    exitStatus DetectRC = Success;             // means nimh of correct polarity is found
    float shuntMA;

    LoadBus();                                 // slight load removes ~1.3V stray from '219 on empty bus

    if (busV > 1.35) {
        DetectRC = Alkaline;                   // 1.35 < alkaline < 1.60
    }
    if (busV > 1.60) {                         // 1.60 < lithium AA < 2.0
        DetectRC = Lithium;
    }
    if (busV > 2.0) {                          // > 2.0, wtf?bp
        DetectRC = UnknownBattery;
    }
    if (busV < 0.1) {
        LightOn();                              // a load which passes thru the '219 shunt
        Monitor(&shuntMA, NULL);
        if (shuntMA < 0) {                      // reverse current comes from reverse polarity battery
            DetectRC = ReversedBattery;
        }
        LightOff();
    }
    UnLoadBus();
    if (digitalRead(BatDetect) == HIGH) {
        DetectRC = NoBattery;
    }
    return DetectRC;                        // if batt detect pin is low, batt is present
                                            // ..also, 0.0 < busV < 1.35, assume nimh
}


//---------------------------------------------------------------------------------------
//    ConstantVoltage -- Charge at a steady voltage until the given time limit is
//                       exceeded, or any of various charging anomalies occurs.
//---------------------------------------------------------------------------------------

exitStatus ConstantVoltage (float targetV, unsigned int minutes, float mAmpFloor)
{
    exitStatus bailRC;
    unsigned long timeStamp;
    const float closeEnough = 0.01;
    float shuntMA, busV, ambientTemp, batteryTemp;

    SetVoltage(targetV - 0.050);
    StartTimer(MaxChargeTimer, (minutes * 60.0));

    while (IsRunning(MaxChargeTimer)) {
        timeStamp = millis();
        Monitor(&shuntMA, &busV);
        GetTemperatures(&batteryTemp, &ambientTemp);
        bailRC = BailOutQ();

        if (bailRC != 0)
            return bailRC;
        if (shuntMA > MAmpCeiling)
            return MaxAmp;
        if (shuntMA < mAmpFloor)
            return MinAmp;

        while (busV < (targetV - closeEnough)) {
            if (NudgeVoltage(+1) == 0)
                return UpperBound;
            Monitor(&shuntMA, &busV);
        }
        while (busV > (targetV + closeEnough)) {
            if (NudgeVoltage(-1) == 0)
                return LowerBound;
            Monitor(&shuntMA, &busV);
        }
        if (HasExpired(ReportTimer))
            CTReport(typeCVRecord, shuntMA, busV, batteryTemp, ambientTemp, timeStamp);

    }
    return MaxTime;
}


//---------------------------------------------------------------------------------------
//    ThermMonitor -- Observe thermometer data (plus current & voltage)
//---------------------------------------------------------------------------------------

exitStatus ThermMonitor (int minutes)
{
    float shuntMA, busV, ambientTemp, batteryTemp;

    ResyncTimer(ReportTimer);
    StartTimer(MaxChargeTimer, (minutes * 60.0));

    while (IsRunning(MaxChargeTimer)) {
        if (Serial.available() > 0)
            return ConsoleInterrupt;

        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            GetTemperatures(&batteryTemp, &ambientTemp);
            CTReport(typeThermRecord, shuntMA, busV, batteryTemp, ambientTemp, millis());
        }
    }
    return MaxTime;
}


//---------------------------------------------------------------------------------------
//    Discharge
//---------------------------------------------------------------------------------------

exitStatus Discharge(float thresh1, float thresh2, unsigned reboundTime)
{
    float shuntMA, busV;
    unsigned long start = millis();

    PowerOff();    // Ensure TLynx power isn't just running down the drain.

//  HeavyOn();
    MediumOn();                  // reduce load to approx that of standalone discharger
    LightOn();
    Monitor(&shuntMA, &busV);
    while (busV > thresh1) {
        if (Serial.available() > 0) {
            //  HeavyOff();
            MediumOff();
            LightOff();
            DisReport(shuntMA, busV, millis());
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer))
            DisReport(shuntMA, busV, millis());
        Monitor(&shuntMA, &busV);
    }
//  HeavyOff();
    MediumOff();
    LightOff();
    StartTimer(ReboundTimer, reboundTime);   // mmm, no console escape during rebound...
    while (IsRunning(ReboundTimer)) {
        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            DisReport(shuntMA, busV, millis());
        }
    }

    LightOn();
    Monitor(&shuntMA, &busV);
    while (busV > thresh2) {
        if (Serial.available() > 0) {
            LightOff();
            DisReport(shuntMA, busV, millis());
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer))
            DisReport(shuntMA, busV, millis());
        Monitor(&shuntMA, &busV);
    }

    LightOff();
    StartTimer(ReboundTimer, reboundTime);   // mmm, no console escape during rebound...
    while (IsRunning(ReboundTimer)) {        // add'l rebound for following scripted cmds
        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            DisReport(shuntMA, busV, millis());
        }
    }

    DisReport(shuntMA, busV, millis());
    return Success;
}


//---------------------------------------------------------------------------------------
//    ResistQ  --
//---------------------------------------------------------------------------------------

float ResistQ (unsigned delayMS)
{
    float shunt1stMA, bus1stV, ohms;
    float shunt2ndMA, bus2ndV, denom;

    Monitor(NULL, &bus1stV);
    SetVoltage(bus1stV + 0.100);
    PowerOn();  delay(delayMS);
    Monitor(&shunt1stMA, &bus1stV);
    PowerOff();  delay(delayMS);
    Monitor(&shunt2ndMA, &bus2ndV);
    denom = (shunt1stMA - shunt2ndMA);
    if (denom == 0.0) denom = 0.001;
    ohms = abs((1000.0 * (bus2ndV - bus1stV)) / denom);
    Printf("%1.4f ohms charge resistance\n", ohms);
    LightOn(); delay(delayMS);
    Monitor(&shunt1stMA, &bus1stV);
    denom = (shunt2ndMA - shunt1stMA);
    if (denom == 0.0) denom = 0.001;
    ohms = abs((1000.0 * (bus2ndV - bus1stV)) / denom);
    Printf("%1.4f ohms internal resistance\n", ohms);
    LightOff(); delay(10);
}


//---------------------------------------------------------------------------------------
//    CoolDown  --
//---------------------------------------------------------------------------------------

exitStatus CoolDown (unsigned int durationM)
{
    float shuntMA, busV, batteryTemp, ambientTemp;

    ResyncTimer(ReportTimer);
    StartTimer(MaxChargeTimer, (durationM * 60.0));

    while (IsRunning(MaxChargeTimer)) {
        if (Serial.available() > 0)
            return ConsoleInterrupt;

        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            GetTemperatures(&batteryTemp, &ambientTemp);
            CTReport(typeCCRecord, shuntMA, busV, batteryTemp, ambientTemp, millis());
        }
    }
    return MaxTime;
}
