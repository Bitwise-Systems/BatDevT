//
//      Print.ino  --  Collection of routines associated with report printing
//
//      Version 2: Removed support for multiple print formats,
//                 ReportExitStatus reads text from EEPROM,
//                 Datasets formatted as Mathematica associations.
//
//      Future work:
//          Meaningful run number
//          Eliminate PotLevel in end record
//

void PrintChargeParams (float target, int minutes, boolean pulsed)
{
    Printf("AddParm[%%runNum->\"BattID=%s, Target=%1.1f, Minutes=%d, Pulsed=%s\"];\n",
        battID, target, minutes, (pulsed) ? "True" : "False");
}


unsigned long StartChargeRecords (void)
{
    Printx("AddData[%runNum->{");
    return millis();
}


void EndChargeRecords (unsigned long startingTime, exitStatus rc)
{
    ldiv_t qr = ldiv(((millis() - startingTime) / 1000L), 60L);

    Printf("{%d, \"%lu mins %lu secs elapsed\", \"Pot: %d\", ",
            typeEndRecord, qr.quot, qr.rem, GetPotLevel());

    ReportExitStatus(rc);
    Printx("}}];\n");

}


void PrintDischargeParams (void)
{
    float busV;

    Monitor(NULL, &busV);
    Printf("AddParm[%%runNum->\"BattID=%s, StartVoltage=%1.3f, Platform=Integrated\"];\n",
            battID, busV);
    Printx("AddData[%runNum->{");

}


void EndDischargeRecords (void)
{
    Printf("{%d}}];\n", typeEndRecord);
}


void CTReport (int type, float shuntMA, float busV, float thermLoad, float thermAmbient, unsigned long millisecs)
{
    Printf("{%d,%1.3f,%1.4f,%1.4f,%1.4f,%lu},\n", type, shuntMA, busV, thermLoad, thermAmbient, millisecs);
}


void DisReport (float shuntMA, float busV, unsigned long millisecs)
{
    Printf("{%d,%1.3f,%1.4f,%lu},\n", typeDischargeRecord, shuntMA, busV, millisecs);
}


void ReportExitStatus (exitStatus rc)
{
    Printx("\"");
    PrintEEPROMstring(rc);
    Printx("\"");

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
