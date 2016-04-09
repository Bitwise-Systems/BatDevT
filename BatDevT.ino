//
//    BatDevT.ino -- NiMH battery charger development system
//
//    Version 4.5: TLynx DC-DC converter with 1024-step level setting
//                 potentiometer & ideal diode backflow prevention,
//                 plus INA219 voltage/current monitor & dual thermometers.
//

#include <Utility.h>
#include <FastDigital.h>
#include <BatDevLocale.h>
#include <EEPROM.h>
#include <DallasTemperature.h>
#include "BatDevT.h"
#include "DriverTimer.h"
#include "Smoothing.h"

const struct DispatchTable commandTable[] = {
    { "b",          SetID           },
    { "bc",         SetCapacity     },
    { "bp",         BatPresentCmd   },
    { "bt",         BatteryTypeCmd  },
    { "cc",         ccCmd           },
    { "comp",       CompileCmd      },
    { "d",          DischargeCmd    },
    { "dump",       DumpRAM         },
    { "heat",       ReportHeats     },
    { "help",       PrintHelp       },
    { "iget",       iGetCmd         },
    { "j",          JugsResetCmd    },
    { "list",       ListCmd         },
    { "memq",       MemQCmd         },
    { "off",        PwrOffCmd       },
    { "on",         PwrOnCmd        },
    { "p",          Precharge       },
    { "pgood",      PwrGoodCmd      },
    { "power",      ExternalPowerQ  },
    { "r",          ResistCmd       },
    { "run",        RunCmd          },
    { "ver",        VersionCmd      },
    { "vget",       vGetCmd         },
    { "vset",       VsetCmd         },

//  { "cv",         cvCmd           },  // Seldom used; saving some space
//  { "getpga",     GetPgaCmd       },
//  { "load",       LoadCmd         },
//  { "nudge",      NudgeCmd        },
//  { "ram",        FreeRam         },
//  { "s",          Sleep219        },
//  { "setpga",     PgaCmd          },
//  { "tell",       Report          },
//  { "thermo",     ThermLoop       },

    {  NULL,        unknownCommand  }   // Insert additional commands BEFORE this line
};


void setup (void)
{
    Serial.begin(38400);
    delay(600);

    InitTLynx();
    if (Init219() == false) {
        Printf("INA219 not found!\n");
        while (1)
            ;
    }
    if (InitThermo() != 0) {
        Printf("Thermometers not found!\n");
        while (1)
            ;
    }
    InitTimerTask(RefreshTemperatures);
    InitLoads();
    SetPGA(8);
    Jugs(NULL, ResetJugs);

    VersionCmd(NULL);
    SetID(NULL);        // Print the default battery ID as a
                        // ...reminder to set the actual one.
#if Locale == Hutch
    ExternalPowerQ(NULL);
#endif
}


void loop (void)
{
    int i;
    char **t;
    exitStatus rc;

//  Printf("> ");
    Printf("\x1B[1G\x1B[0K> ");    // cursor to column 1, and erase to end of line
    t = util.GetCommand();
    if (*t == NULL)
        return;
    for (i = 0; commandTable[i].command != NULL; i++)
        if (strcmp(commandTable[i].command, *t) == 0)
            break;

    rc = (*commandTable[i].handler)(t);    // call command handler

    if (rc != 0) {
        CommentExitStatus(rc);
		Printf("\n");
    }
    if (strcmp(commandTable[i].command, "run") == 0)    // send the 'quit' escape sequence
        Printf("$Q");

}
