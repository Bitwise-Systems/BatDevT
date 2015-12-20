//---------------------------------------------------------------------------------------
//      Script.ino  --  Script interpreter & library
//---------------------------------------------------------------------------------------

//    Command line repertoire:

char *defaultCharge[] = {"cc", NULL};
char *topOffCharge[] = {"cc", "10", "20", NULL};        // rate = C/10 for 20 minutes
char *discharge[] = {"d", "0.8", "1.0", "480", NULL};
char *internalR[] = {"r", NULL};
char *battPresent[] = {"bp", NULL};


//    Scripts:

char **standard[] = {
    battPresent,
    internalR,
    discharge,
    internalR,
    defaultCharge,
    internalR,
    discharge,
    NULL
};

char **topOff[] = {     // Test efficacy of including a "top-off" charge:
    battPresent,
    internalR,
    discharge,          //   discharge to a known starting point,
    internalR,
    defaultCharge,      //   charge without topping off,
    internalR,
    discharge,          //   discharge for jugs, & to get back to known start;
    internalR,
    defaultCharge,      //   charge...
    topOffCharge,       //   ...and then top off,
    internalR,
    discharge,          //   discharge for jugs.
    NULL
};


//    Library:

char ***scripts[] = {
    standard,           // script 0
    topOff              // script 1
};


//---------------------------------------------------------------------------------------


#define MaxIndex (((sizeof scripts) / (sizeof scripts[0])) - 1)

exitStatus ScriptCmd (char **args)      // Script command processor
{
    exitStatus rc;
    static exitStatus goodRC[] = {Success, DipDetected, MaxTime, ChargeTempThreshold, SENTINEL};

    int i, j, k = (*++args == NULL) ? 0 : constrain(atoi(*args), 0, MaxIndex);

    for (j = 0; scripts[k][j] != NULL; j++) {
        for (i = 0; commandTable[i].command != NULL; i++)
            if (strcmp(commandTable[i].command, *scripts[k][j]) == 0)
                break;

        rc = (*commandTable[i].handler)(scripts[k][j]);

        if (! memberQ(rc, goodRC)) {
            Printf("Untoward return code from '%s' (rc = %d), ", *scripts[k][j], rc);
            return rc;
        }
    }
    return Success;

}


static boolean memberQ (exitStatus element, exitStatus *set)    // Set membership query
{
    for (int i = 0; set[i] != SENTINEL; i++)
        if (element == set[i])
            return true;

    return false;

}
