//---------------------------------------------------------------------------------------
//    DriverThermo.ino  --  Basic support for two Dallas Semiconductor DS18B20
//                          thermometer chips. If installed in a polling environment,
//                          'RefreshTemperatures' should be called frequently. In a
//                          timer-task setting, it should be installed as the timer
//                          tick callback function.
//---------------------------------------------------------------------------------------

#include <OneWire.h>
#include <DallasTemperature.h>

#define ThermoResolution 12        // Thermometer resolution (in bits)

#define ThermoConvertTime (750 / (1 << (12-ThermoResolution)))

OneWire oneWire(OneWireBus);
DallasTemperature thermo(&oneWire);

static DeviceAddress thermoAddr0, thermoAddr1;      // DS18B20 devices addresses.
static volatile float temperature0, temperature1;   // Most recent temperature readings.
                                                    // Initialized in 'InitThermo',
                                                    // updated by 'RefreshTemperatures',
                                                    // available through 'GetTemperatures'.
static unsigned long thermoAvailable;               // System time at which a new...
                                                    // ...temperature conversion is...
                                                    // ...complete & available to be read.
int InitThermo (void)
{
    thermo.begin();

    if (!thermo.getAddress(thermoAddr0, 0))
        return 1;

    if (!thermo.getAddress(thermoAddr1, 1))
        return 2;

    thermo.setResolution(thermoAddr0, ThermoResolution);
    thermo.setResolution(thermoAddr1, ThermoResolution);

    thermo.setWaitForConversion(true);
    thermo.requestTemperatures();
    temperature0 = thermo.getTempC(thermoAddr0);
    temperature1 = thermo.getTempC(thermoAddr1);

    thermo.setWaitForConversion(false);     // From here on out, non-blocking conversion
    thermo.requestTemperatures();
    thermoAvailable = millis() + ThermoConvertTime;
    return 0;

}


void RefreshTemperatures (void)    // Timer tick callback
{
    if (millis() > thermoAvailable) {
        temperature0 = thermo.getTempC(thermoAddr0);
        temperature1 = thermo.getTempC(thermoAddr1);
        thermo.requestTemperatures();
        thermoAvailable = millis() + ThermoConvertTime;
    }
}


void GetTemperatures (float *t0, float *t1)
{
    cli();
    *t0 = temperature1;                 // probes swapped in hardware version 4
    *t1 = temperature0;
    sei();

}


void PrintThermoAddress (DeviceAddress addr)
{
    for (int i = 0; i < 8; i++) {
        if (addr[i] < 16)
            Serial.write('0');
        Serial.print(addr[i], HEX);
        Serial.write(',');
    }
}


int GetThermoIdent (char **args)
{
    Printx("Thermometer 0: ");
    PrintThermoAddress(thermoAddr0);
    Printx("  Thermometer 1: ");
    PrintThermoAddress(thermoAddr1);
    Printx("\n");
    return 0;

}

