//
//      Commands.ino  --  Handlers for interpreter commands
//

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

#define ToMinutes(div) ((57.0 * div) + 45.0)    // map C-rate to charging minutes

exitStatus ccCmd (char **args)
{
    float busV;
    exitStatus rc;
    unsigned long startRecordsTime;

    float divisor = 4.0;                      // Default C-rate = C/4
    float targetMA = capacity / divisor;
    float minutes = ToMinutes(divisor);

    int i = 0;
    while (*++args != NULL) {
        switch (++i) {
          case 1:
            divisor = constrain(atof(*args), 1.0, 50.0);
            targetMA = capacity / divisor;
            minutes = ToMinutes(divisor);
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
    Printf("Voltage just before setvoltage: %1.3f\n", busV);
    SetVoltage(busV + 0.050);                                 // Final value should be slightly
    PowerOn();                                                // ...over battery voltage.
//  delay(20);
    Monitor(NULL, &busV);
    Printf("Voltage just after setvoltage: %1.3f\n", busV);
    startRecordsTime = millis();

    do {
        PrintChargeParams(divisor, targetMA, minutes);
        StartChargeRecords();
        rc = ConstantCurrent(targetMA, minutes);
        Jugs(NULL, ReportJugs);
        SetVoltage(1.1);                   // otherwise pot setting stays too high
        divisor *= 1.5;                    // downshift to 2/3 of previous charge rate
        targetMA = capacity / divisor;
        minutes = (1.0 - (Jugs(0.0, ReturnCharge)/capacity)) * ToMinutes(divisor);
        ReportExitStatus(rc);
        delay(10);                                  // <<< ?? >>>
        EndChargeRecords(startRecordsTime, rc);
    } while ((divisor < 31) && (rc == PanicVoltage || rc == UpperBound));

    PowerOff();
    delay(10);
    SetVoltage(SetVLow);

    return rc;

}


//------------------------------------------------------------------------------
//    cvCmd -- Constant voltage command handler
//
//    Usage: cv [targetV [minutes [floor]]]
//
//------------------------------------------------------------------------------

// exitStatus cvCmd (char **args)
// {
//     exitStatus rc;
//     unsigned long startRecordsTime;
//
//     float targetV = SetVLow;        // defaults...
//     unsigned minutes = 10;
//     float mAmpFloor = 2.0;
//
//     int i = 0;
//     while (*++args != NULL) {
//         switch (++i) {
//           case 1:
//             targetV = constrain(atof(*args), SetVLow, SetVHigh);
//             break;
//           case 2:
//             minutes = constrain(atoi(*args), 1, 1440);
//             break;
//           case 3:
//             mAmpFloor = constrain(atof(*args), 1.0, MAmpCeiling);
//             break;
//         }
//     }
// //  Formerly, a 'battery present check' and an 'external power check' occurred here.
// //  These are now available as standalone commands, and can be incorporated into any script.
//
//     SetVoltage(SetVLow);
//     PowerOn();
//     PrintChargeParams(0.0, targetV, minutes);
//     startRecordsTime = StartChargeRecords();
//     rc = ConstantVoltage(targetV, minutes, mAmpFloor);
//     Jugs(NULL, ReportJugs);
//     if (rc != 0)
//         ReportExitStatus(rc);
//     EndChargeRecords(startRecordsTime, rc);
//     PowerOff();
//     SetVoltage(SetVLow);
//
//     return rc;
//
// }


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

    float thresh1 = 0.8;      // Defaults...
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

    if ((rc = discharge(thresh1, LoadByCapacity)) == Success)
      if ((rc = rebound(reboundSecs)) == Success)
        if ((rc = discharge(thresh2, LightOn)) == Success)
          if ((rc = rebound(reboundSecs)) == Success)
              ;
    Jugs(NULL, ReportJugs);
    EndDischargeRecords();

    return Success;

}


exitStatus ExternalPowerQ (char **args)
{
    float busV;
    exitStatus rc;

    SetVoltage(SetVLow);
    PowerOn();
    Monitor(NULL, &busV);
    SetVoltage(busV + 0.2);
    rc = (StatusQ() == 0) ? Success : IdealDiodeStatus;
    PowerOff();
    SetVoltage(SetVLow);

    if (rc != Success)
        Printf("\nNo external power\n\n");

    return rc;

}

// exitStatus FreeRam (char **args)
// {
//     int v;
//     extern int __heap_start, *__brkval;
//
//     v = (int) &v - ((__brkval == 0) ? (int) &__heap_start : (int) __brkval);
//     Printf("Free SRAM: %d bytes\n", v);
//     return Success;
//
// }

// exitStatus GetPgaCmd (char **args)
// {
//     (void) args;
//     Printf("PGA divisor: %d\n", GetPGA());
//     return Success;
// }


exitStatus iGetCmd (char **args)
{
    float shuntMA;

    Monitor(&shuntMA, NULL);
    Printf("{%d, \"Shunt current: %1.1fmA\"},\n", typeInfo, shuntMA);
    return Success;
}

exitStatus JugsResetCmd (char **args)
{
    Jugs(NULL, ResetJugs);
//  ResetCoulombCounter();
    Printf("{%d, \"resetJugsTally\"},\n", typeInfo);
    return Success;

}


// exitStatus LoadCmd (char **args)
// {
//     int i;
//     static struct {
//         const char *command;
//         void (*handler)(void);
//
//     } loadTable[] = {
//         { "+h",    HeavyOn   },
//         { "-h",    HeavyOff  },
//         { "+m",    MediumOn  },
//         { "-m",    MediumOff },
//         { "+l",    LightOn   },
//         { "-l",    LightOff  },
//         { "+b",    LoadBus   },
//         { "-b",    UnLoadBus },
//         { "-all",  AllLoadsOff },
//         { NULL,    UnknownLoad }
//     };
//
//     while (*++args != NULL) {
//         for (i = 0; loadTable[i].command != NULL; i++)
//             if (strcmp(loadTable[i].command, *args) == 0)
//                 break;
//
//         (*loadTable[i].handler)();
//     }
//     LoadStatus();
//     return Success;
//
// }


// exitStatus NudgeCmd (char **args)
// {
//     Printf("Nudged by %d\n", NudgeVoltage((*++args == NULL) ? 1 : atoi(*args)));
//     return Success;
// }


// exitStatus PgaCmd (char **args)
// {
//     int divisor;
//
//     if (*++args != NULL) {
//         divisor = atoi(*args);
//         switch (divisor) {
//           case 1:
//           case 2:
//           case 4:
//           case 8:
//             SetPGA(divisor);
//             break;
//           default:
//             Printf("Invalid divisor.  Need 1, 2, 4, or 8.\n");
//             break;
//         }
//     }
//     Printf("PGA divisor is %d\n", GetPGA());
//     return Success;
// }


//------------------------------------------------------------------------------
//    Precharge -- Prepare battery for constant current charging
//
//    Usage: p [voltageFloor [resistanceCeiling
//                     [targetVoltage [prechargeMinutes]]]]
//
//------------------------------------------------------------------------------

exitStatus Precharge (char **args)
{
    exitStatus rc;
    float busV, chargingResistance;
    unsigned long startRecordsTime;

    float voltageFloor = 0.8;          // defaults...
    float resistanceCeiling = 3.0;
    float targetV = 1.6;
    unsigned minutes = 5;

    int i = 0;
    while (*++args != NULL) {
        switch (++i) {
          case 1:
            voltageFloor = constrain(atof(*args), 0.0, 1.0);
            break;
          case 2:
            resistanceCeiling = constrain(atof(*args), 0.0, 10.0);
            break;
          case 3:
            targetV = constrain(atof(*args), 1.1, 1.7);
            break;
          case 4:
            minutes = constrain(atoi(*args), 1, 30);
            break;
        }
    }
    Monitor(NULL, &busV);
    if (busV > 1.30)
        return Success;        // no need to precharge  <<< should this be loaded? >>>

    startRecordsTime = millis();
    while (minutes < 40) {          // continue precharge if there is too little effect
        PrintChargeParams(0.0, targetV, minutes);
        StartChargeRecords();
        SetVoltage(SetVLow);
        PowerOn();
        rc = ConstantVoltage(targetV, minutes, 1);    // <<< What about BailQ? >>>
        PowerOff();
        Jugs(NULL, ReportJugs);
        EndChargeRecords(startRecordsTime, rc);
        ReportExitStatus(rc);
        delay(15 * 1000);                             // allow surface charge to fade
        chargingResistance = ResistQ(ChargeIt, 1000);
        if (chargingResistance < 1.1)
            break;
        minutes += 30;
    }
    SetVoltage(SetVLow);
    if ((rc != Success) || (rc != MaxTime))     // <<< turn MaxTime to Success >>>
        ReportExitStatus(rc);
    return rc;

}


exitStatus PrintHelp (char **args)      // Provided in BatDev.py, which should
{                                       // ...intercept the command
    (void) args;

    Printf("Help unavailable\n");
    return Success;

}


exitStatus PwrGoodCmd (char **args)
{
    (void) args;

    Printf("PGood is %sokay\n", (PowerGoodQ() == 1) ? "" : "not ");
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


exitStatus ReportHeats (char **args)
{
    float thermLoad, thermAmbient;

    GetTemperatures(&thermLoad, &thermAmbient);
    Printf("Battery temp: %1.2f   Ambient temp: %1.2f\n", thermLoad, thermAmbient);
    return Success;

}

#define ChargeIt 0
#define DischargeIt 1

exitStatus ResistCmd (char **args)
{
    (void) args;

    ResistQ(ChargeIt, 1000);
    ResistQ(DischargeIt, 1000);
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
    Printf("{%d, \"Bus Voltage: %1.3fV\"},\n", typeInfo, busV);
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
