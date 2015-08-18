//
//      Commands.ino  --  Handlers for interpreter commands
//
//      Remove large data headers from ccdCmd and cvCmd and replace the functionality
//      ...with PrintCCInfo and CVInfo   9/3/14
//
//      Replace individual load on/off commands with single cmd and arg 6/30/15
//      Remove some testing commands:
//      -- tuglines, looking for who is causing extra voltage on bus
//      -- thCmd, monitors temps and reports
//
//      Reduced FreeRam from two types of ram reports to one
//      Tracked extra voltage on bus (with no battery) to INA219
//
//

exitStatus ThermLoop (char **args)    // Temporarily provide access to 'ThermMonitor'
{
    int minutes = (*++args == NULL) ? 1 : atoi(*args);
    (void) ThermMonitor(minutes);
    Printx("Done\n");
    return Success;
}


exitStatus BatPresentCmd (char **args)
{
    exitStatus rc;
    float busV;

    Monitor(NULL, &busV);
    rc = BatteryPresentQ(busV);
    ReportExitStatus(rc);
    Printx("\n");
    return Success;
}


exitStatus ccdCmd (char **args)     // calls constant current  arg1 is target MA, arg 2 is minutes
{
    float shuntMA, busV, voltCeiling;
    int targetMA, minutes;
    unsigned long startRecordsTime;
    exitStatus exitRC = Success;

    targetMA =    (*++args == NULL) ? 100        : constrain(atoi(*args), 0, 4000);  // max 4A to protect 2W .1ohm shunt resistor
    minutes =     (*++args == NULL) ? 10         : constrain(atoi(*args), 1, 1440);    //..4A x .4V drop = 1.6W
    voltCeiling = (*++args == NULL) ? maxBatVolt : constrain(atof(*args), SetVLow, SetVHigh);

    Monitor(NULL, &busV);
    Printf("just before setvoltage volts: %1.3f\n", busV);     // <<testing initial high voltage>>
    exitRC = BatteryPresentQ(busV);
    if (exitRC != 0)   return exitRC;
    SetVoltage(busV + 0.020);      // final val should be slightly under batt voltage  ?remove .01 bump?
    PowerOn();
    delay(10);
    Monitor(&shuntMA, NULL);
    Monitor(NULL, &busV);                                     // <<testing initial high voltage>>

    Printf("(* Predicted end-of-ramp voltage: %f *)\n", (targetMA * busV) / shuntMA);

    Printf("just after setvoltage volts: %1.3f\n", busV);     // <<testing initial high voltage>>
    if (StatusQ() == 1)  {       // if so, could be lack of external power
        Printx("No external power, exiting\n");
        return DiodeTrip;
    }

    PrintCCInfo (targetMA, minutes);

    startRecordsTime = StartRecords();
    exitRC = ConstantCurrent(targetMA, minutes, voltCeiling);   //testing Mikey's cc
    EndRecords(startRecordsTime, exitRC);
    PowerOff();
    SetVoltage(SetVLow);
   // CoolDown(15);
    Printx("~");
    return exitRC;
}


exitStatus cvCmd (char **args)     // calls constant voltage  arg1 is target Volts, arg 2 is minutes
{                           // ..on arg3 constantvoltage bails at or below this value
    float shuntMA, busV, targetV, mAmpFloor;
    int minutes;
    unsigned long startRecordsTime;
    exitStatus rc;

    targetV =    (*++args == NULL) ? SetVLow  : constrain(atof(*args), SetVLow, SetVHigh);
    minutes =     (*++args == NULL) ? 10      : constrain(atoi(*args), 1, 1440);
    mAmpFloor = (*++args == NULL) ?  5.0      : constrain(atof(*args), 1, 4000);  // max safes shunt resistor

    Monitor(&shuntMA, &busV);
    if (busV < 0.1) {                                   // don't power up
        Printx("Don't see a battery, exiting\n");
        return MinV;                 // change this, cmd handler doesn't do exitStatus
    }
    if  (shuntMA < -80.0)  {
        Printx("No external power, exiting");
        return NegMA;
    }

    PrintCVInfo (targetV, minutes);

    SetVoltage(SetVLow);      // start at lowest, let constant voltage ramp it up
    PowerOn();
    startRecordsTime = StartRecords();

    do {
        rc = ConstantVoltage(targetV, minutes, mAmpFloor);
        if (rc != 0)
            ReportExitStatus(rc);
        delay(5000);
        Monitor(NULL, &busV);
        targetV += 0.1;
    } while (targetV < 1.5);

    EndRecords(startRecordsTime, rc);
    PowerOff();
    SetVoltage(SetVLow);
    Printf("~\n");
    return Success;
}


exitStatus cvCPR (char **args)          // bring a battery back from the dead
{
    exitStatus rc;

    for (int i = 0; i < 4; i++) {
        rc = ConstantVoltage(1.0, 5, 2);
        if (rc = MaxTime)
            continue;
        else if (rc = MinAmp)
            break;
        else {
            ReportExitStatus(rc);
            return ParameterError;
        }
    }
    for (int i = 0; i < 4; i++) {
        rc = ConstantVoltage(1.1, 10, 2);
        if (rc = MaxTime)
            continue;
        else if (rc = MinAmp)
            break;
        else {
            ReportExitStatus(rc);
            return ParameterError;
        }
    }
    for (int i = 0; i < 4; i++) {
        rc = ConstantVoltage(1.2, 15, 2);
        if (rc = MaxTime)
            continue;
        else if (rc = MinAmp)
            break;
        else {
            ReportExitStatus(rc);
            return ParameterError;
        }
    }
    for (int i = 0; i < 4; i++) {
        rc = ConstantVoltage(1.3, 20, 10);
        if (rc = MaxTime)
            continue;
        else if (rc = MinAmp)
            break;
        else {
            ReportExitStatus(rc);
            return ParameterError;
        }
    }
    for (int i = 0; i < 4; i++) {
        rc = ConstantVoltage(1.4, 20, 20);
        if (rc = MaxTime)
            continue;
        else if (rc = MinAmp)
            break;
        else {
            ReportExitStatus(rc);
            return ParameterError;
        }
    }
    return Success;
}


exitStatus DischargeCmd (char **args)
{
    return Discharge(0.8, 1.0, 480);    // 8 minute (480 seconds) rebound time
}


#define RAMSize 2048        // ATmega328P

exitStatus FreeRam (char **args)
{
    byte *heap = NULL;
    int n = RAMSize;

    while (n > 0 && (heap = (byte *) malloc(n)) == NULL)
        n--;
    free(heap);
    Printf("Avail SRAM: %d bytes\n", n);
    return Success;
}


exitStatus GetPgaCmd (char **args)
{
    (void) args;
    Printf("PGA presently set to %d\n", GetPGA());
    return Success;
}


exitStatus iGetCmd (char **args)
{
    float shuntMA;

    Monitor(&shuntMA, NULL);
    Printf("Bus current: %1.1fmA\n", shuntMA);
    return Success;
}


exitStatus LoffCmd(char **args)
{
    switch (**++args) {
    case 'h':
        HeavyOff();
        Printx("hi ");
        break;
    case 'm':
        MediumOff();
        Printx("med ");
        break;
    case 'l':
        LightOff();
        Printx("low ");
        break;
    case 'b':
        UnLoadBus();
        Printx("bus");
        break;
    default:
        Printx("!arg No ");
        break;
    }
    Printx("load off\n");
    return Success;
}


exitStatus LonCmd(char **args)
{
    switch (**++args) {
    case 'h':
        HeavyOn();
        Printx("hi ");
        break;
    case 'm':
        MediumOn();
        Printx("med ");
        break;
    case 'l':
        LightOn();
        Printx("low ");
        break;
    case 'b':
        LoadBus();
        Printx("bus");
        break;
    default:
        Printx("!arg No ");
        break;
    }
    Printx("load on\n");
    return Success;
}


exitStatus NudgeCmd (char **args)
{
    Printf("Nudged by %d\n", NudgeVoltage((*++args == NULL) ? 1 : atoi(*args)));
    return Success;
}


exitStatus PgaCmd (char **args)
{
    int divisor;

    if (*++args != NULL) {
        divisor = atoi(*args);
        switch (divisor) {
          case 1:
          case 2:
          case 4:
          case 8:
            SetPGA(divisor);
            break;
          default:
            Printx("Invalid divisor.  Need 1, 2, 4, or 8.");
            break;
        }
    }
    Printf("PGA divisor is %d\n", GetPGA());
    return Success;
}


exitStatus PrintHelp (char **args)
{
    char c;
    int i;
    for (i = 0; (c = EEPROM.read(i)) != '\0'; i++)
        Serial.write(c);
    return Success;
}


exitStatus PwrGoodCmd (char **args)
{
    (void) args;
    Printf("PGood is %s\n", PowerGoodQ() == 1 ? "okay" : "not okay");
    return Success;
}


exitStatus PwrOnCmd (char **args)
{
    (void) args;

    PowerOn();
    Printx("Power ON\n");
    return Success;
}


exitStatus PwrOffCmd (char **args)
{
    (void) args;

    PowerOff();
    Printx("Power OFF\n");
    return Success;
}


exitStatus Report (char **args)
{
    (void) args;
    float busV, shuntMA, thermLoad, thermAmbient;

    Monitor(&shuntMA, &busV);
    GetTemperatures(&thermLoad, &thermAmbient);
    CTReport(shuntMA, busV, thermLoad, thermAmbient, millis(), 0);
    return Success;
}


exitStatus ReportHeats (char **args)
{
    float thermLoad, thermAmbient;

    GetTemperatures(&thermLoad, &thermAmbient);
    Printf("Load Temp: %1.2f   Ambient Temp: %1.2f\n", thermLoad, thermAmbient);
    return Success;
}


exitStatus SetID (char **args)   // would rather this do strings than a single char....
{                // doesn't constrain to alpha chars
    battID = (*++args == NULL) ? battID : **args;
    battID = toupper(battID);
    Printf("BattID: %c\n", battID);
    return Success;
}

// function SetPrintFormat is under 'Print' tab

/*
int SetRunNum (char **args)
{
    runNum = (*++args == NULL) ? runNum : atoi(*args);
    Printf("runNum: %d\n", runNum);
    return 0;
}
*/


exitStatus unknownCommand (char **args)
{
    (void) args;

    Printx("Don't know that command\n");
    return ParameterError;

}


exitStatus vGetCmd (char **args)
{
    float busV;

    Monitor(NULL, &busV);
    Printf("Bus Voltage: %1.3fV\n", busV);
    return Success;
}


exitStatus VsetCmd (char **args)
{
    float voltsetting;

    voltsetting = SetVoltage((*++args == NULL) ? SetVLow : atof(*args));
    Printf("setting prelim voltage to %1.3f\n", voltsetting);
    return Success;

}
