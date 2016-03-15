//---------------------------------------------------------------------------------------------------
//    Drivers.h  --  Constants and magic numbers for the power management module.
//
//    Version 4 hardware
//
//---------------------------------------------------------------------------------------------------

#ifndef _Drivers_h_
#define _Drivers_h_

//    Arduino pin assignments. I2C pins are not actually manipulated by
//    driver code. They are reserved by, and managed by, the Wire library.

#define HeavyLoadGate   2    // Drives MOSFET to apply heavy (2.0 ohm) load to battery
#define MediumLoadGate  3    // Drives MOSFET to apply medium (4.7 ohm) load to battery
#define LightLoadGate   4    // Drives MOSFET to apply light (10 ohm) load to battery

#define BatDetect       6    // Batt pulls int_pullup down to Low (< 1.5 V)
#define BusLoad         7    // Grounds (4.3K) resistor from busbar
#define DiodeStatus     8    // LTC4352 ideal diode status pin: 0 => MOSFET on
#define PowerON         9    // TLynx 'ON' pin: active HIGH
#define PotDirection   11    // MAX5483 count direction: HIGH increments wiper
#define OneWireBus     12    // DS18B20 data bus
#define PotToggle      13    // MAX5483 clock pin: falling edge

#define PowerGood      A3    // TLynx 'PGD' pin: 0 => power is NOT good
#define I2C_SDA        A4    // INA219 data bus
#define I2C_SCL        A5    // INA219 clock pin


//    Supply voltage limits:

#define SetVLow  0.94798      // Lowest permissible output voltage  11-22-15
#define SetVHigh 1.85287      // Highest permissible output voltage


//    Scaling coefficients for converting raw ADC readings to bus voltage and current:

typedef struct { float busScale; float busOffset; float shuntScale; float shuntOffset; } coList;


//    Miscellaneous:

#define ShuntResistor_Ohms 0.1    // On-board shunt resistor: 0.1 Ohm, 1%, 1 (or 2?) Watt

typedef enum {          // Function return codes used throughout the application:
    SENTINEL = -1,      //     Used to terminate exitStatus lists for memberQ
    Success = 0,
    ParameterError,
    ConsoleInterrupt,
    UpperBound,
    LowerBound,
    PBad,
    PanicVoltage,          // max permitted voltage on individual session
    MaxChargeVoltage,      // max voltage ever allowed
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
    Lithium

} exitStatus;

#endif _Drivers_h_
