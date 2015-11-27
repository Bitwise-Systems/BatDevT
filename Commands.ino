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
//      "help" command functionality moved to terminal monitor
//
//

exitStatus BatPresentCmd (char **args)
{
    exitStatus rc;
    float busV;

    Monitor(NULL, &busV);
    rc = BatteryPresentQ(busV);
    ReportExitStatus(rc);
    Printf("\n");
    return Success;

}


exitStatus ccCmd (char **args)     // Arg1 is targetMA, arg2 is minutes
{
    exitStatus exitRC;
    int targetMA, minutes;
    unsigned long startRecordsTime;
    float shuntMA, busV;

    targetMA = (*++args == NULL) ? 400 : constrain(atoi(*args), 0, MAmpCeiling);    // protect shunt resistor
    minutes =  (*++args == NULL) ? 360 : constrain(atoi(*args), 1, 1440);

    Monitor(NULL, &busV);
    Printf("Voltage just before setvoltage: %1.3f\n", busV);     // <<testing initial high voltage>>
    exitRC = BatteryPresentQ(busV);
    if (exitRC != 0)
        return exitRC;
    SetVoltage(busV + 0.050);      // final val should be slightly under batt voltage  ?remove .01 bump?
    PowerOn();
    delay(10);
    Monitor(&shuntMA, &busV);                                 // <<testing initial high voltage>>
    Printf("Voltage just after setvoltage: %1.3f\n", busV);   // <<testing initial high voltage>>

    if (StatusQ() == 1)  {       // if so, could be lack of external power
        PowerOff();
        Printf("No external power, exiting\n");
        return IdealDiodeStatus;
    }
    PrintChargeParams(targetMA, minutes, false);      // False means no pulsing
    startRecordsTime = StartChargeRecords();
    exitRC = ConstantCurrent(targetMA, minutes);
    EndChargeRecords(startRecordsTime, exitRC);
    PowerOff();
    SetVoltage(SetVLow);
    if (! scriptrunning)
        Printf("~");
    return exitRC;

}

exitStatus cvCmd (char **args)     // Calls ConstantVoltage. Arg1 is target volts,
{                                  // ...arg2 is minutes, arg3: bail at or below this value
    int minutes;
    exitStatus rc;
    unsigned long startRecordsTime;
    float shuntMA, busV, targetV, mAmpFloor;

    targetV =   (*++args == NULL) ? SetVLow   : constrain(atof(*args), SetVLow, SetVHigh);
    minutes =   (*++args == NULL) ? 10        : constrain(atoi(*args), 1, 1440);
    mAmpFloor = (*++args == NULL) ? 5.0       : constrain(atof(*args), 1, 4000);  // max safes shunt resistor

    Monitor(&shuntMA, &busV);
    if (busV < 0.1) {
        Printf("Don't see a battery; exiting\n");
        return NoBattery;
    }
    if  (shuntMA < -80.0)  {
        Printf("No external power; exiting");
        return IdealDiodeStatus;
    }

    PrintChargeParams(targetV, minutes, false);
    SetVoltage(SetVLow);        // Let ConstantVoltage ramp it up
    PowerOn();
    startRecordsTime = StartChargeRecords();
    do {
        rc = ConstantVoltage(targetV, minutes, mAmpFloor);
        if (rc != 0)
            ReportExitStatus(rc);
        delay(5000);
        Monitor(NULL, &busV);
        targetV += 0.1;
    } while (targetV < 1.5);

    EndChargeRecords(startRecordsTime, rc);
    PowerOff();
    SetVoltage(SetVLow);
    Printf("~\n");
    return Success;

}


exitStatus DischargeCmd (char **args)
{
    exitStatus rc;
    float thresh1, thresh2;
    unsigned int reboundSecs;

    thresh1 =     (*++args == NULL) ? 0.8 : constrain(atof(*args), 0.5, 1.4);
    thresh2 =     (*++args == NULL) ? 1.0 : constrain(atof(*args), 0.5, 1.4);
    reboundSecs = (*++args == NULL) ? 480 : constrain(atoi(*args), 1, 6000);

    PrintDischargeParams();
    rc = Discharge(thresh1, thresh2, reboundSecs);
    EndDischargeRecords();

    if (! scriptrunning)
        Printf("~");
    return rc;

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


exitStatus LoffCmd(char **args)
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


exitStatus LonCmd(char **args)
{
    if (*++args != NULL) {
        switch (**++args) {
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
    float busV, shuntMA, thermLoad, thermAmbient;

    Monitor(&shuntMA, &busV);
    GetTemperatures(&thermLoad, &thermAmbient);
    CTReport(-1, shuntMA, busV, thermLoad, thermAmbient, millis());
    return Success;

}


exitStatus ReportHeats (char **args)
{
    float thermLoad, thermAmbient;

    GetTemperatures(&thermLoad, &thermAmbient);
    Printf("Battery temp: %1.2f   Ambient temp: %1.2f\n", thermLoad, thermAmbient);
    return Success;

}


exitStatus ResistCmd (char **args)          // in mid-level
{
    (void) args;

    ResistQ(1000);
    delay(5000);
    return Success;

}


exitStatus ScriptCmd (char **args)
{
    exitStatus rc;

    static char *chargeCmd[] = {"cc","600", NULL};
//  static char *chargeCmd[] = {"cc","400", NULL};   // Default
//  static char *chargeCmd[] = {"cc","1750", NULL};  // D-cell

    static char *dischCmd[] = {"d", "0.8", "1.0", "480", NULL};   // phase 1 lower limit, phase 2..
                                                                  //..lower limit, rebound seconds
    scriptrunning = true;                                         // std, 0.8-1.0-480

    ResistQ(1000);
    rc = DischargeCmd(dischCmd);
    if (rc != Success) {
       scriptrunning = false;
       Printf("Premature exit from discharge\n");
       return rc;
    }
    ResistQ(1000);
    rc = ccCmd(chargeCmd);
    if ((rc != DipDetected) && (rc != MaxTime) && (rc != ChargeTempThreshold)) {
        scriptrunning = false;
        Printf("Premature exit from constant charge\n");
        return rc;
    }
    ResistQ(1000);
    rc = DischargeCmd(dischCmd);
    if (rc != Success) {
        scriptrunning = false;
        Printf("Premature exit from discharge\n");
        return rc;
    }
    ResistQ(1000);
    rc = ccCmd(chargeCmd);
    if ((rc != DipDetected) && (rc != MaxTime) && (rc != ChargeTempThreshold)) {
        scriptrunning = false;
        Printf("Premature exit from constant charge\n");
        return rc;
    }
    scriptrunning = false;
    Printf("~");
    return Success;

}


exitStatus SetID (char **args)
{
    if ((args != NULL) && (*++args != NULL))
        strncpy(battID, *args, (sizeof battID) - 1);

    Printf("Battery ID: %s\n", battID);
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
