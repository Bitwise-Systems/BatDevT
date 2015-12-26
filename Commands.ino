//
//      Commands.ino  --  Handlers for interpreter commands
//

exitStatus ExternalPowerQ (char **args)    // <<< EVALUATING >>>
{
    float busV;
    exitStatus rc;

    SetVoltage(SetVLow);
    PowerOn();
    delay(20);
    Monitor(NULL, &busV);
    SetVoltage(busV + 0.2);
    rc = (StatusQ() == 0) ? Success : IdealDiodeStatus;
    PowerOff();
    SetVoltage(SetVLow);

    Printf("External power is %s\n", (rc == 0) ? "ON" : "OFF");

    return rc;

}


exitStatus BatPresentCmd (char **args)
{
    return BatteryPresentQ();

}


exitStatus BatteryTypeCmd (char **args)
{
    return BatteryTypeQ();

}


//------------------------------------------------------------------------------
//    ccCmd -- Constant current command handler
//
//    Usage: cc [C-rate [minutes [targetMA]]]
//
//------------------------------------------------------------------------------

exitStatus ccCmd (char **args)
{
    float busV;
    exitStatus rc;
    unsigned long startRecordsTime;

    float divisor = 4.0;                      // Default C-rate = C/4
    float targetMA = capacity / divisor;
    float minutes = (57.0 * divisor) + 45.0;

    int i = 0;
    while (*++args != NULL) {
        switch (++i) {
          case 1:
            divisor = constrain(atof(*args), 1.0, 20.0);
            targetMA = capacity / divisor;
            minutes = (57.0 * divisor) + 45.0;
            break;
          case 2:
            minutes = atof(*args);
            break;
          case 3:
            targetMA = constrain(atof(*args), 1.0, MAmpCeiling);
            divisor = capacity / targetMA;
            break;
        }
    }
    minutes = constrain(minutes, 1.0, 1440.0);
    Printf("C-rate = %1.1f; minutes = %1.1f; targetMA = %1.1f\n", divisor, minutes, targetMA);

    Monitor(NULL, &busV);
    Printf("Voltage just before setvoltage: %1.3f\n", busV);  // <<< testing initial high voltage >>>
    SetVoltage(busV + 0.050);                                 // Final value should be slightly
    PowerOn();                                                // ...under battery voltage.
    delay(20);
    Monitor(NULL, &busV);                                     // <<< testing initial high voltage >>>
    Printf("Voltage just after setvoltage: %1.3f\n", busV);   // <<< testing initial high voltage >>>

//  Formerly, a check for external power occurred here. This is now
//  available as a standalone command, and can be incorporated into any script.

    PrintChargeParams(targetMA, minutes);
    startRecordsTime = StartChargeRecords();
    rc = ConstantCurrent(targetMA, minutes);
    EndChargeRecords(startRecordsTime, rc);
    PowerOff();
    SetVoltage(SetVLow);

    return rc;

}


//------------------------------------------------------------------------------
//    cvCmd -- Constant voltage command handler
//
//    Usage: cv [targetV [minutes [floor]]]
//
//------------------------------------------------------------------------------

exitStatus cvCmd (char **args)
{
    exitStatus rc;
    unsigned long startRecordsTime;

    float targetV = SetVLow;        // defaults...
    unsigned minutes = 10;
    float mAmpFloor = 5.0;

    int i = 0;
    while (*++args != NULL) {
        switch (++i) {
          case 1:
            targetV = constrain(atof(*args), SetVLow, SetVHigh);
            break;
          case 2:
            minutes = constrain(atoi(*args), 1, 1440);
            break;
          case 3:
            mAmpFloor = constrain(atof(*args), 1.0, MAmpCeiling);
            break;
        }
    }
//  Formerly, a 'battery present check' and an 'external power check' occurred here.
//  These are now available as standalone commands, and can be incorporated into any script.

    PrintChargeParams(targetV, minutes);
    SetVoltage(SetVLow);
    PowerOn();
    delay(20);
    startRecordsTime = StartChargeRecords();
    do {
        rc = ConstantVoltage(targetV, minutes, mAmpFloor);
        if (rc != 0)
            ReportExitStatus(rc);
        delay(5000);
        targetV += 0.1;
    } while (targetV < 1.5);

    EndChargeRecords(startRecordsTime, rc);
    PowerOff();
    SetVoltage(SetVLow);

    return Success;

}


//------------------------------------------------------------------------------
//    DischargeCmd -- Discharge command handler
//
//    Usage: d [threshold1 [threshold2 [reboundTime]]]
//
//------------------------------------------------------------------------------

exitStatus DischargeCmd (char **args)
{
    void LoadByCapacity(void), LightOn(void);
    exitStatus rc, discharge(float threshold, void (*loadFunction)());

    float thresh1 = 0.8;            // defaults...
    float thresh2 = 1.0;
    unsigned reboundSecs = 480;

    int i = 0;
    while (*++args != NULL) {
        switch (++i) {
          case 1:
            thresh1 = constrain(atof(*args), 0.5, 1.4);
            break;
          case 2:
            thresh2 = constrain(atof(*args), 0.5, 1.4);
            break;
          case 3:
            reboundSecs = constrain(atoi(*args), 1, 6000);
            break;
        }
    }

    PowerOff();               // Ensure TLynx power isn't just running down the drain.
    PrintDischargeParams();
    if ((rc = discharge(thresh1, LoadByCapacity)) != Success)
        return rc;
    if ((rc = rebound(reboundSecs)) != Success)
        return rc;
    if ((rc = discharge(thresh2, LightOn)) != Success)
        return rc;
    if ((rc = rebound(reboundSecs)) != Success)
        return rc;
    EndDischargeRecords();
    AllLoadsOff();

    return Success;

}

//------------------------------------------------------------------------------

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
    Printf("PGA divisor: %d\n", GetPGA());
    return Success;
}


exitStatus iGetCmd (char **args)
{
    float shuntMA;

    Monitor(&shuntMA, NULL);
    Printf("Shunt current: %1.1fmA\n", shuntMA);
    return Success;
}


exitStatus LoffCmd (char **args)
{
    if (*++args != NULL) {
        switch (**args) {
          case 'h':
            HeavyOff();
            Printf("hi ");
            break;
          case 'm':
            MediumOff();
            Printf("med ");
            break;
          case 'l':
            LightOff();
            Printf("low ");
            break;
          case 'b':
            UnLoadBus();
            Printf("bus");
            break;
          default:
            Printf("!arg No ");
            break;
        }
        Printf("load off\n");
    }
    else
        Printf("no arg, no action\n");
    return Success;
}


exitStatus LonCmd (char **args)
{
    if (*++args != NULL) {
        switch (**args) {
          case 'h':
            HeavyOn();
            Printf("hi ");
            break;
          case 'm':
            MediumOn();
            Printf("med ");
            break;
          case 'l':
            LightOn();
            Printf("low ");
            break;
          case 'b':
            LoadBus();
            Printf("bus");
            break;
          default:
            Printf("!arg No ");
            break;
        }
        Printf("load on\n");
    }
    else
        Printf("no arg, no action\n");
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
            Printf("Invalid divisor.  Need 1, 2, 4, or 8.");
            break;
        }
    }
    Printf("PGA divisor is %d\n", GetPGA());
    return Success;
}


exitStatus PrintHelp (char **args)
{
    (void) args;

    Printf("Help unavailable\n");
    return Success;

}


exitStatus PwrGoodCmd (char **args)
{
    (void) args;

    Printf("PGood is %s\n", (PowerGoodQ() == 1) ? "okay" : "not okay");
    return Success;
}


exitStatus PwrOnCmd (char **args)
{
    (void) args;

    PowerOn();
    Printf("Power ON\n");
    return Success;
}


exitStatus PwrOffCmd (char **args)
{
    (void) args;

    PowerOff();
    Printf("Power OFF\n");
    return Success;
}


exitStatus Report (char **args)
{
    (void) args;
    float busV, shuntMA;

    Monitor(&shuntMA, &busV);
    GenReport(typeCC, shuntMA, busV, millis());
    return Success;

}


exitStatus ReportHeats (char **args)
{
    float thermLoad, thermAmbient;

    GetTemperatures(&thermLoad, &thermAmbient);
    Printf("Battery temp: %1.2f   Ambient temp: %1.2f\n", thermLoad, thermAmbient);
    return Success;

}


exitStatus ResistCmd (char **args)
{
    (void) args;

    ResistQ(1000);
    delay(5000);
    return Success;

}


exitStatus SetID (char **args)
{
    if ((args != NULL) && (*++args != NULL))
        strncpy(battID, *args, (sizeof battID) - 1);

    Printf("Battery ID: %s\n", battID);
    return Success;

}


exitStatus SetCapacity (char **args)
{
    int mA = capacity;

    if ((args != NULL) && (*++args != NULL))
        mA = atoi(*args);

    if (mA < 500)
        Printf("Error: capacity too low\n");
    else
        capacity = mA;

    Printf("\rBattery mAH: %d\n", capacity);
    return Success;

}


exitStatus ThermLoop (char **args)    // Provide access to 'ThermMonitor'
{
    int minutes = (*++args == NULL) ? 1 : atoi(*args);
    (void) ThermMonitor(minutes);
    Printf("Done\n");
    return Success;
}


exitStatus unknownCommand (char **args)
{
    (void) args;

    Printf("Don't know that command\n");
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
    Printf("Setting voltage to %1.3f\n", voltsetting);
    return Success;

}


exitStatus VersionCmd (char **args)
{
    static char *date = __DATE__;
    static char *time = __TIME__;

    Printf("BatDevT (version: %s, %s)\n", date, time);
    return Success;

}
