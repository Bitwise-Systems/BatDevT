//---------------------------------------------------------------------------------------------------
//    BatDevT.h  --  NiMH battery charger
//---------------------------------------------------------------------------------------------------

#ifndef _BatDevT_h_
#define _BatDevT_h_

//    Arduino pin assignments. I2C pins are not actually manipulated by
//    driver code. They are reserved by, and managed by, the Wire library.

#define HeavyLoadGate   2    // Drives MOSFET to apply heavy (2.0 ohm) load to battery
#define MediumLoadGate  3    // Drives MOSFET to apply medium (4.7 ohm) load to battery
#define LightLoadGate   4    // Drives MOSFET to apply light (10 ohm) load to battery

#define BatDetect       6    // Battery presence pulls internal pull-up down (< 1.5 V)
#define BusLoad         7    // Grounds (4.3K) resistor from V- power bus
#define DiodeStatus     8    // LTC4352 ideal diode status pin: 0 => MOSFET on
#define PowerON         9    // TLynx 'ON' pin: active HIGH
#define PotDirection   11    // MAX5483 count direction: HIGH increments wiper
#define OneWireBus     12    // DS18B20 data bus
#define PotToggle      13    // MAX5483 clock pin: falling edge

#define PowerGood      A3    // TLynx 'PGD' pin: 0 => power is NOT good
#define I2C_SDA        A4    // INA219 data bus
#define I2C_SCL        A5    // INA219 clock pin


//    Report record types:

#define typeCC         0     // CTRecord format: shuntMA, busV, thermLoad, thermAmbient, millisecs
#define typeCV         1     // Use CTRecord format
#define typeDetect     2     // Dip Detected; CTRecord format
#define typeTherm      3     // Use CTRecord format
#define typeRampUp     4     // Used in ConstantCurrent during ramp-up phase
#define typePulse      5     // Presently unused
#define typeDischarge  9     // Use CTRecord format
#define typeEnd       10     // Elapsed time, exitStatus, et. al.
#define typeProvEnd   11     // not written yet, use EndRecord format
#define typeJugs      12     // Charge & discharge mAh report
#define typeNudge     13     // nudgeCount, potLevel, millis
#define typeIRes      14     // Internal Resistance record, ohms
#define typeInfo      15     // Information notices

//    "ResistQ" routine command codes:

#define ChargeIt       0     // Request charging resistance
#define DischargeIt    1     // Request discharging resistance

//    "Jugs" routine command codes:

typedef enum {
    Tally,
    ReportJugs,
    ResetJTime,
    ResetJugs,
    ReturnCharge,
    ReturnDischarge

} JugsCommand;

//    Function return-codes used throughout the application:

typedef enum {
    SENTINEL = -1,      // Used to terminate exitStatus lists for memberQ
    Success = 0,
    ParameterError,
    ConsoleInterrupt,
    UpperBound,
    LowerBound,
    PBad,
    PanicVoltage,       // Max permitted voltage on individual session
    MaxChargeVoltage,   // Max voltage ever allowed
    MaxAmp,
    MinAmp,
    PanicTemp,
    ChargeTempThreshold,
    ChargeTempRate,
    MaxTime,
    IdealDiodeStatus,
    DipDetected,
    NoBattery,
    ReversedBattery,
    UnknownBattery,
    Alkaline,
    Lithium,
    Accepting
} exitStatus;


//    Scaling coefficients for converting raw ADC readings to bus
//    voltage and shunt current:

typedef struct {
    float busScale;
    float busOffset;
    float shuntScale;
    float shuntOffset;
} coList;


//    Command dispatcher table entry format:

struct DispatchTable {
    const char *command;                  // Command name
    exitStatus (*handler)(char **);       // Pointer to command handler
};


//    Miscellaneous:

#define SetVLow   0.94798    // Lowest permissible output voltage  (11-22-15)
#define SetVHigh  1.85287    // Highest permissible output voltage
#define ShuntR_Ohms   0.1    // 0.1 Ohm, 1%, 1 (or 2?) Watt
#define MAmpCeiling  3000    // Max allowable thru INA219 shunt resistor
#define MaxBatTemp   40.9    // Max allowed battery temperature (deg C)
#define MaxBatVolt   1.72    // Max allowed battery voltage
#define ChargeTemp   8.00    // Charge is complete if delta-T exceeds 8.0 deg C
#define BandPlus     7.00    // ConstantCurrent upper band width (mA)
#define BandMinus    3.00    // ConstantCurrent lower band width (mA)


//    Global variables:

char battID[20] = "<undefined>";          // See '~/MSR/BatDevInventory.py'
int capacity = 2400;                      // Battery capacity in mAh


#endif _BatDevT_h_
