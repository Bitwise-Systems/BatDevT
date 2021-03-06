//=======================================================================================
//    MidLevel.ino -- Repository for routines that perform a significant amount
//                    of work in connecting the command handlers to the low-level
//                    drivers.
//=======================================================================================

//---------------------------------------------------------------------------------------
//    BailOutQ  --
//---------------------------------------------------------------------------------------

exitStatus BailOutQ (void)
{
    float busV, batteryTemp, ambientTemp;

    Monitor(NULL, &busV);
    GetTemperatures(&batteryTemp, &ambientTemp);

    exitStatus bailRC = Success;       // success means no bail-outs tripped

    if (batteryTemp > MaxBatTemp)   bailRC = PanicTemp;
    if (busV > MaxBatVolt)          bailRC = PanicVoltage;
    if (!PowerGoodQ())              bailRC = PBad;
    if (StatusQ() == 1)             bailRC = IdealDiodeStatus;
    if (Serial.available() > 0)     bailRC = ConsoleInterrupt;
    if (bailRC != Success)
        Printf("(* Bail! %1.3f V, %1.2f deg *)\n", busV, batteryTemp);

    return bailRC;

}


//---------------------------------------------------------------------------------------
//    BatteryPresentQ  --
//---------------------------------------------------------------------------------------

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
      if (digitalReadFast(BatDetect) == HIGH) {    // BatDetect pulled high via internal 20K
          rc = NoBattery;
      }
      RemoveImpress();
      return rc;                        // if batdetect pin is low, batt is pulling pin down

  }

#else
  #error "Unknown locale in MidLevel.ino"

#endif


//---------------------------------------------------------------------------------------
//    BatteryTypeQ  --
//---------------------------------------------------------------------------------------

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

exitStatus ConstantVoltage (float targetV, unsigned minutes)
{
    exitStatus bailRC;
    float shuntMA, busV;
    unsigned long timeStamp;
    const float closeEnough = 0.01;

    StartTimer(MaxChargeTimer, (minutes * 60.0));
    ResyncTimer(ReportTimer);
    Jugs(NULL, ResetJTime);
    SetVoltage(targetV);

    while (IsRunning(MaxChargeTimer)) {
        timeStamp = millis();
        Monitor(&shuntMA, &busV);
        Jugs(shuntMA, Tally);

        if ((bailRC = BailOutQ()) != 0)
            return bailRC;
        if (shuntMA > MAmpCeiling)
            return MaxAmp;
        if (shuntMA < 1.0)
            return MinAmp;
        if ((shuntMA > (capacity / 10.0)) && (minutes > 5))    // <<< TESTING (minutes > 5) >>>
            return Accepting;                                  // Absorbing at greater than C/10 implies
                                                               //...battery is ready for constant current charging
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
//    Jugs --
//---------------------------------------------------------------------------------------

float Jugs (float milliAmps, int action)
{
    static unsigned long jugMillis;
    unsigned long localMillis;
    static float jCTally = 0.0;             // charge tally, in micro-coulombs (mA*mS)
    static float jDTally = 0.0;             // discharge tally

    switch (action) {
      case Tally:
          localMillis = millis();
          if (milliAmps > 0.0)
              jCTally += (milliAmps) * (localMillis - jugMillis);
          else
              jDTally -= (milliAmps) * (localMillis - jugMillis);
          jugMillis = localMillis;
          break;
      case ReportJugs:
          Printf("{%d, \"%1.2f mAh in, %1.2f mAh out\"},\n", typeJugs,
             jCTally / (60.0 * 60.0 * 1000.0), jDTally / (60.0 * 60.0 * 1000.0));
          break;
      case ResetJTime:
          jugMillis = millis();
          break;
      case ResetJugs:                       // ?can't do 0.0 exactly?
          jCTally = 0.0;
          jDTally = 0.0;
          break;
      case ReturnCharge:
          return jCTally / (60.0 * 60.0 * 1000.0);
      case ReturnDischarge:
          return jDTally / (60.0 * 60.0 * 1000.0);
      default:
          Printf("JugsWTF?\n");
    }
    return 0.0;
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
//    rebound  --  Twiddle thumbs waiting for battery to rebound from discharging
//---------------------------------------------------------------------------------------

exitStatus rebound (unsigned reboundTime)
{
    float shuntMA, busV;

    AllLoadsOff();
    ResyncTimer(ReportTimer);
    StartTimer(ReboundTimer, reboundTime);

    while (IsRunning(ReboundTimer)) {
        if (Serial.available() > 0) {
            while (Serial.read() > -1)
                ;
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer)) {
            Monitor(&shuntMA, &busV);
            GenReport(typeDischarge, shuntMA, busV, millis());
        }
    }
    return Success;

}


//---------------------------------------------------------------------------------------
//    discharge  --  Discharge down to a given voltage through a specified load
//---------------------------------------------------------------------------------------

exitStatus discharge (float threshold, void (*loadFunction)())
{
    float shuntMA, busV;

    (*loadFunction)();
    ResyncTimer(ReportTimer);
    Jugs(NULL, ResetJTime);

    Monitor(&shuntMA, &busV);
    while (busV > threshold) {
        if (Serial.available() > 0) {
            while (Serial.read() > -1)
                ;
            AllLoadsOff();
            return ConsoleInterrupt;
        }
        if (HasExpired(ReportTimer))
            GenReport(typeDischarge, shuntMA, busV, millis());

        Monitor(&shuntMA, &busV);
        Jugs(shuntMA, Tally);
    }
    AllLoadsOff();
    return Success;

}


//---------------------------------------------------------------------------------------
//    ResistQ  --
//---------------------------------------------------------------------------------------

float ResistQ (boolean resisttype, unsigned delayMS)
{
    float shunt1stMA, bus1stV, ohms;
    float shunt2ndMA, bus2ndV, denom;

    if (resisttype == ChargeIt) {
        Monitor(NULL, &bus1stV);
        SetVoltage(bus1stV + 0.100);
        PowerOn();
    }
    else {
        LightOn();
    }
    delay(delayMS);
    Printf("{%d, \"", typeInfo);
    Monitor(&shunt1stMA, &bus1stV);
    if (resisttype == ChargeIt) {
        PowerOff();
        Printf("charge ");
    }
    else {
        LightOff();
        Printf("discharge ");
    }
    delay(delayMS);
    Monitor(&shunt2ndMA, &bus2ndV);
    denom = (shunt1stMA - shunt2ndMA);
    if (denom == 0.0) denom = 0.001;
    ohms = abs((1000.0 * (bus2ndV - bus1stV)) / denom);
    Printf("resistance: %1.4f ohms\"},\n", ohms);

    return ohms;

}


//---------------------------------------------------------------------------------------
//   MaintCharge  -- Maintenance charge. Maintain charge of already fully charged
//                   battery that is left in the holder for an extended period.
//---------------------------------------------------------------------------------------

void MaintCharge (void)
{
    float busV;

    while (BatteryPresentQ() == Success) {
         Monitor(NULL, &busV);
        if (busV < 1.35) {
            ConstantCurrent(capacity/10, 10);
        }
    }
}


//---------------------------------------------------------------------------------------
//    CoolDown  --
//---------------------------------------------------------------------------------------
//
// exitStatus CoolDown (unsigned int durationM)
// {
//     float shuntMA, busV;
//
//     ResyncTimer(ReportTimer);
//     StartTimer(MaxChargeTimer, (durationM * 60.0));
//
//     while (IsRunning(MaxChargeTimer)) {
//         if (Serial.available() > 0)
//             return ConsoleInterrupt;
//
//         if (HasExpired(ReportTimer)) {
//             Monitor(&shuntMA, &busV);
//             GenReport(typeCC, shuntMA, busV, millis());
//         }
//     }
//     return Success;
// }
