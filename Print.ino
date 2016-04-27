//
//      Print.ino  --  Collection of routines associated with report printing
//
//      Version 2: Removed support for multiple print formats,
//                 ReportExitStatus reads text from EEPROM,
//                 Datasets formatted as Mathematica associations.
//
//


void PrintChargeParams (float cRate, float target, int minutes)
{
    Printf("AddParm[%%runNum->\"BattID=%s, CRate=%1.1f, Target=%1.1f, Minutes=%d\"];\n",
        battID, cRate, target, minutes);
}


void PrintDischargeParams (void)
{
    float busV;

    Monitor(NULL, &busV);
    Printf("AddParm[%%runNum->\"BattID=%s, StartVoltage=%1.3f, Platform=Integrated\"];\n",
            battID, busV);
    Printf("AddData[%%runNum->{");

}


unsigned long StartChargeRecords (void)
{
    Printf("AddData[%%runNum->{");
    return millis();
}


void EndChargeRecords (unsigned long startingTime, exitStatus rc)
{
    ldiv_t qr = ldiv(((millis() - startingTime) / 1000L), 60L);

    ReportExitStatus(rc);
    Printf("{%d, \"%lu mins %lu secs elapsed\", \"Pot: %d\"}}];\n",
            typeEnd, qr.quot, qr.rem, GetPotLevel());

}


void EndDischargeRecords (void)
{
    Printf("{%d}}];\n", typeEnd);
}


void GenReport (int recordType, float shuntMA, float busV, unsigned long millisecs)
{
    float batteryTemp, ambientTemp;

    GetTemperatures(&batteryTemp, &ambientTemp);

    Printf("{%d,%1.3f,%1.4f,%1.4f,%1.4f,%lu},\n",
        recordType, shuntMA, busV, batteryTemp, ambientTemp, millisecs);

}

void CommentExitStatus (exitStatus rc)
{
    Printf("(* ");
    PrintEEPROMstring(rc);
    Printf(" *)");

}


void ReportExitStatus (exitStatus rc)
{
    Printf("{%d, \"", typeInfo);
    PrintEEPROMstring(rc);
    Printf("\"},\n");

}


void PrintEEPROMstring (int n)    // Print nth string from EEPROM (zero-based)
{
    char c;
    int i = 0;

    while (n-- > 0)
        while (i < 1023 && EEPROM.read(i++) != '\0')    // ATmega328P capacity = 1024 bytes
            continue;
    while ((c = EEPROM.read(i++)) != '\0')
        Serial.write(c);

}
