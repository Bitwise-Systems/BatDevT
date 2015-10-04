//
//      Print.ino  --  Collection of routines associated with report printing
//

char *printNames[] = {
    "NoPrint",
    "Console",
    "Math",
    "DataBox",
    "CSV",
};

#define NprintNames (sizeof(printNames)/sizeof(char*))

typedef enum  { NoPrint, Console, Math, DataBox, CSV } outputFormat;

outputFormat printformat = Math;


exitStatus SetPrintFormat (char **args) {

    if (*++args != NULL) {
        if (strcmp(*args, "none") == 0)            printformat = NoPrint;
        else if (strcmp(*args, "noprint") == 0)    printformat = NoPrint;
        else if (strcmp(*args, "console") == 0)    printformat = Console;
        else if (strcmp(*args, "math") == 0)       printformat = Math;
        else if (strcmp(*args, "databox") == 0)    printformat = DataBox;
        else if (strcmp(*args, "csv") == 0)        printformat = CSV;
        else
            Printx("unrecognized parameter");
    }
    Printf("Print format is %s\n", printNames[printformat]);
    return Success;
}

unsigned long StartRecords (void)
{
    Printf("runData[%%runNum]:={");
    return millis();
}

void EndRecords (unsigned long startingTime, exitStatus rc)
{
    int minutes;
    int seconds;
    unsigned long elapsed = (millis() - startingTime);

    minutes = elapsed / (60 * 1000UL);
    seconds = (elapsed % (60 * 1000UL))/1000;

    Printf("{%d, \"%d mins %d secs elapsed\", \"Pot: %d\", ", typeEndRecord, minutes, seconds, GetPotLevel());
    ReportExitStatus(rc);
    Printx("}};\n\n");
}

<<<<<<< HEAD
=======
void PrintDischargeStart (void)
{
    float busV;
    Monitor(NULL, &busV);
    Printf("runParams[%%runNum]:=\"BattID=%s, StartVoltage=%1.3f, Platform=Integrated\";\n", battID, busV);  // discharge info
    Printf("dischargeData[%%runNum]:={");                // start records
    //IRreport(InternalResistance());
}
>>>>>>> 520724dd604b35027790f7e69151be7cb6e7f2f8

void DisReport (float shuntMA, float busV, unsigned long millisecs)
{  
    Printf("{9, %1.3f, %1.4f, %lu},\n", shuntMA, busV, millisecs);    
}    


void CTReport (int type, float shuntMA, float busV, float thermLoad, float thermAmbient, unsigned long millisecs)
{
#define ctr_variables type, shuntMA, busV, thermLoad, thermAmbient, millisecs

    switch (printformat) {
    case NoPrint:
        break;
    case Console:
        Printf("type: %d  shuntMA: %1.1f  busV: %1.4f  Load: %1.4f  Amb: %1.4f  mSecs: %lu\n", ctr_variables);
        break;
    case Math:
        Printf("{%d,%1.1f,%1.4f,%1.4f,%1.4f,%lu},\n", ctr_variables);
        break;
    case DataBox:
        Printf("0:%d1:1.1f\n2:%1.4f\n3:%1.4f\n4:%1.4f\n5:%lu\n", ctr_variables);
        break;
    case CSV:
        Printf("%d,%1.1f,%1.4f,%1.4f,%1.4f,%lu\n", ctr_variables);
        break;
    default:
        Printx("unknown print format");
        break;
    }
}

void PrintCCInfo (float targetMA, int minutes)
{
    Printf("runParams[%%runNum]:=\"BattID=%s, targetmA=%1.1f, minutes=%d, band=+%d/-%d, Platform=Integrated\";\n", battID, targetMA, minutes, (int)bandPlus, (int)bandMinus);
}

void PrintCVInfo (float targetV, int minutes)
{
    Printf("runParams[%%runNum]:=\"BattID=%s, targetV=%1.1f, minutes=%d, Platform=Integrated\";\n", battID, targetV, minutes);
}

void NudgeReport (int nudgeCount, int potLevelBN, unsigned long timestamp)
{
    Printf("\n{%d,%d,%d,%lu},\n", typeNudgeRecord, nudgeCount, potLevelBN, timestamp);
}

void ReportExitStatus (exitStatus rc)
{
    Printx("\"");          // Provide printed quotes around status for M'tica compat
    switch (rc) {
      case Success:           Printx("Success");             break;
      case ParameterError:    Printx("Param err");           break;
      case SystemError:       Printx("Syst err");            break;
      case ConsoleInterrupt:  Printx("Console'rupt");        break;
      case BoundsCheck:       Printx("Bounds");              break;
      case PBad:              Printx("PGood Bad");           break;
      case MinV:              Printx("Min Volt");            break;
      case MaxV:              Printx("Max Volt");            break;
      case TripV:             Printx("Thresh Volt");         break;
      case DeltaV:            Printx("Volt Delta");          break;
      case MaxAmp:            Printx("Max Amp");             break;
      case MinAmp:            Printx("Min Amp");             break;
      case MaxTemp:           Printx("Max Temp");            break;
      case DeltaTemp:         Printx("Temp Delta");          break;
      case MaxTime:           Printx("Timed Out");           break;
      case NegMA:             Printx("Shunt too neg");       break;
      case DiodeTrip:         Printx("Ideal Diode Off");     break;
      case NoBatt:            Printx("No Battery");          break;
      case BatRev:            Printx("Reverse Polarity");    break;
      case UnkBatt:           Printx("Unknown Batt Type");   break;
      case Alky:              Printx("Alkaline Battery");    break;
      case Lithi:             Printx("Lithium Battery");     break;
      default:                Printx("Default exitStatus");  break;
    }
    Printx("\"");
//  Printx("\n");       // commented out so we can use it within termination record
}
