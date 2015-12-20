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

#if Locale == Mike
  exitStatus BatteryPresentQ (void)
  {
      float busV;
      exitStatus rc;

      AllLoadsOff();
      LoadBus();         // Slight resistive load reduces
      delay(500);        // ...phantom '219 bus voltage.

      Monitor(NULL, &busV);
      rc = (busV < 0.4) ? NoBattery : Success;

      UnLoadBus();
      return rc;

  }

#elif Locale == Hutch
  exitStatus BatteryPresentQ (void)
  {
      exitStatus rc = Success;

      AllLoadsOff();
      Impress();
      if (digitalRead(BatDetect) == HIGH) {    // BatDetect pin pulled high via internal 20K
          rc = NoBattery;
      }
      RemoveImpress();
      return rc;                        // if batdetect pin is low, batt is pulling pin down

  }

#else
  #error "Unknown locale in MidLevel.ino"

#endif


exitStatus BatteryTypeQ (void)
{
    exitStatus rc = Success;           // means NiMH of correct polarity found
    float shuntMA, busV;

    AllLoadsOff();
    LoadBus();                         // Slight resistive load removes
    delay(750);                        // ...phantom '219 voltage from bus.
    Monitor(NULL, &busV);
    if (busV > 1.44) {
        rc = Alkaline;                 // 1.44 < alkaline < 1.60
    }
    if (busV > 1.60) {                 // 1.60 < lithium AA < 2.0
        rc = Lithium;
    }
    if (busV > 2.0) {                  // > 2.0, wtf?
        rc = UnknownBattery;
    }
    UnLoadBus();
    if (busV < 0.1) {
        LightOn();                     // a load which passes thru the '219 shunt
        Monitor(&shuntMA, NULL);
        if (shuntMA < 0) {             // reverse current comes from reverse polarity battery
            rc = ReversedBattery;
        }
        LightOff();
    }
    return rc;                        // if batt detect pin is low, batt is present
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
            GenReport(typeCV, shuntMA, busV, timeStamp);

    }
    return MaxTime;
}


//---------------------------------------------------------------------------------------
//    ThermMonitor -- Observe thermometer data (plus current & voltage)
//---------------------------------------------------------------------------------------

exitStatus ThermMonitor (int minutes)
{
    float shuntMA, busV;

    ResyncTimer(ReportTimer);
    StartTimer(MaxChargeTimer, (minutes * 60.0));

    while (IsRunning(MaxChargeTimer)) {
        if (Serial.available() > 0)
            return ConsoleInterrupt;

        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            GenReport(typeTherm, shuntMA, busV, millis());
        }
    }
    return Success;
}


//---------------------------------------------------------------------------------------
//    Discharge -- Discharge battery
//---------------------------------------------------------------------------------------

exitStatus Discharge (float thresh1, float thresh2, unsigned reboundTime)
{
    float shuntMA, busV;
    float batteryTemp, ambientTemp;

    PowerOff();     // Ensure TLynx power isn't just running down the drain.

    LoadByCapacity();         // Phase-1 discharge
    Monitor(&shuntMA, &busV);
    while (busV > thresh1) {
        if (Serial.available() > 0) {
            AllLoadsOff();
            GenReport(typeDischarge, shuntMA, busV, millis());
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer)) {
            GenReport(typeDischarge, shuntMA, busV, millis());
            GetTemperatures(&batteryTemp, &ambientTemp);
            if (batteryTemp - ambientTemp > 1.0) {
                GenReport(typeTherm, shuntMA, busV, millis());
            }
        }
        Monitor(&shuntMA, &busV);
    }

    AllLoadsOff();      // First rebound
    StartTimer(ReboundTimer, reboundTime);
    while (IsRunning(ReboundTimer)) {
        if (Serial.available() > 0) {
            GenReport(typeDischarge, shuntMA, busV, millis());
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            GenReport(typeDischarge, shuntMA, busV, millis());
        }
    }

    LightOn();          // Phase-2 discharge
    Monitor(&shuntMA, &busV);
    while (busV > thresh2) {
        if (Serial.available() > 0) {
            AllLoadsOff();
            GenReport(typeDischarge, shuntMA, busV, millis());
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer))
            GenReport(typeDischarge, shuntMA, busV, millis());
        Monitor(&shuntMA, &busV);
    }

    AllLoadsOff();      // Second rebound
    StartTimer(ReboundTimer, reboundTime);
    while (IsRunning(ReboundTimer)) {
        if (Serial.available() > 0) {
            GenReport(typeDischarge, shuntMA, busV, millis());
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            GenReport(typeDischarge, shuntMA, busV, millis());
        }
    }

    GenReport(typeDischarge, shuntMA, busV, millis());
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
    float shuntMA, busV;

    ResyncTimer(ReportTimer);
    StartTimer(MaxChargeTimer, (durationM * 60.0));

    while (IsRunning(MaxChargeTimer)) {
        if (Serial.available() > 0)
            return ConsoleInterrupt;

        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            GenReport(typeCC, shuntMA, busV, millis());
        }
    }
    return Success;
}
