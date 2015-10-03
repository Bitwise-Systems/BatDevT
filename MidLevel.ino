//=======================================================================================
//    MidLevel.ino -- Repository for routines that perform a significant amount
//                    of work in connecting the command handlers to the low-level
//                    drivers.
//=======================================================================================


exitStatus BailOutQ (float busV, float battemp)
{
    exitStatus bailRC = Success;       // success means no bail-outs 'tripped'

    if (battemp > maxBatTemp)   bailRC = MaxTemp;
    if (busV > maxBatVolt)      bailRC = MaxV;
    if (!PowerGoodQ())          bailRC = PBad;
    if (StatusQ() == 1)         bailRC = DiodeTrip;     // will trip if used during off pulse
    if (Serial.available() > 0) bailRC = ConsoleInterrupt;
    if (bailRC != Success)
        Printf("Bail! %1.3f V, %1.2f deg\n", busV, battemp);

    return bailRC;

}


exitStatus BatteryPresentQ (float busV)
{
    return Success;         // <<< TEMPORARY: checking effect of removing battery detection circuitry >>>

    exitStatus DetectRC = Success;             // means nimh of correct polarity is found
    float shuntMA;

    LoadBus();                                 // slight load removes ~1.3V stray from '219 on empty bus

    if (busV > 1.35) {
        DetectRC = Alky;                       // 1.35 < alky > 1.60
    }
    if (busV > 1.60) {                         // 1.60 < lithium AA > 2.0
        DetectRC = Lithi;
    }
    if (busV > 2.0) {                          // > 2.0, wtf?bp
        DetectRC = UnkBatt;
    }
    if (busV < 0.1) {
        LightOn();                              // a load which passes thru the '219 shunt
        Monitor(&shuntMA, NULL);
        if (shuntMA < 0) {                      // reverse current comes from reverse polarity battery
            DetectRC = BatRev;
        }
        LightOff();
    }
    UnLoadBus();
    if (digitalRead(BatDetect) == HIGH) {
        DetectRC = NoBatt;
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
        bailRC = BailOutQ(busV, batteryTemp);

        if (bailRC != 0)
            return bailRC;
        if (shuntMA > mAmpCeiling)
            return MaxAmp;
        if (shuntMA < mAmpFloor)
            return MinAmp;

        while (busV < (targetV - closeEnough)) {
            if (NudgeVoltage(+1) == 0)
                return BoundsCheck;
            Monitor(&shuntMA, &busV);
        }
        while (busV > (targetV + closeEnough)) {
            if (NudgeVoltage(-1) == 0)
                return BoundsCheck;
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

exitStatus Discharge (float thresh1, float thresh2, unsigned reboundTime)
{
    float shuntMA, busV;
    //unsigned long start = millis();

    PowerOff();    // Ensure TLynx power isn't just running down the drain.

   // HeavyOn();
    MediumOn();                  // reduce load to approx that of standalone discharger
    LightOn();
    Monitor(&shuntMA, &busV);
    while (busV > thresh1) {
        if (Serial.available() > 0) {
            // HeavyOff();
            MediumOff();
            LightOff();
            DisReport(shuntMA, busV, millis());
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer))
            DisReport(shuntMA, busV, millis());

        Monitor(&shuntMA, &busV);
    }
    // HeavyOff();
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
