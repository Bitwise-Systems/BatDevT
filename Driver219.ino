//---------------------------------------------------------------------------------------------------
//    Driver219.ino -- Low-level driver for the INA219 current monitor chip
//
//    Version 1:  First '219-only version. Equivalent to 'Drivers.ino' version 7
//    Version 2:  Add the capability to put the '219 into power-down mode
//    Version 3:  Add "coulomb counter" to Monitor, plus supporting routines
//
//---------------------------------------------------------------------------------------------------

#include <Wire.h>                           // The INA219 current monitor speaks I2C

#define ConfigRegister 0x00                 // INA219 register addresses...
#define ShuntVoltageRegister 0x01
#define BusVoltageRegister 0x02

#define DefaultPGA 8                        // Default divisor for INA219's programmable gain amplifier

static int chipAddress = 0x00;              // Discovered in 'InitDrivers' by call to 'GetINA219Address'
static float busScale, busOffset;           // Coefficients for converting raw bus ADC value to volts
static float shuntScale, shuntOffset;       // Coeffieients for converting raw shunt ADC value to milliamps
static unsigned configShuntV = 0x1FF9;      // 16V busV, div-8 PGA, 12-bit, 128-sample averaging, triggered, shunt voltage only
static const unsigned configBusV = 0x1C42;  // 16V busV, div-8 PGA, 12-bit, single sample, triggered, bus voltage only


boolean Init219 (void)
{
    coList k;
    float busVoid;

    Wire.begin();
    if ((chipAddress = GetINA219Address()) == 0x00)
        return false;
    SetPGA(DefaultPGA);
    k = FetchConstants(chipAddress, DefaultPGA);
    busScale = k.busScale;            // Bus voltage conversion coefficients
    busOffset = k.busOffset;          // ...will remain unchanged from here on out.
    shuntScale = k.shuntScale;        // Shunt voltage coefficients are updated
    shuntOffset = k.shuntOffset;      // ...any time the PGA divisor changes.
    Monitor(NULL, &busVoid);          // Ensure current monitor chip is in triggered mode.
    return true;

}


int Get219Address (void)
{
    return chipAddress;

}

long long coulombs;
long currentTimestamp, previousTimestamp, firstTimestamp;

void Monitor (float *currentMA, float *busV)
{
    int shuntRaw;
    byte sampleCount;
    unsigned busRaw, accumRaw = 0;

    if (busV != NULL) {
        for (sampleCount = 0; sampleCount < 64; sampleCount++) {    // Collect 4^n samples, n = 3
            Wire.beginTransmission(chipAddress);                    // ...(see oversampling appnote, AVR121).
            Wire.write(ConfigRegister);
            Wire.write(configBusV >> 8);
            Wire.write(configBusV & 0x00FF);
            Wire.endTransmission();                 // Trigger a new BUS voltage ADC conversion.

            Wire.beginTransmission(chipAddress);    // It's complete when the CNVR bit in
            Wire.write(BusVoltageRegister);         // ...the Bus Voltage Register is asserted.
            Wire.endTransmission();
            do {
                Wire.requestFrom(chipAddress, 2);
                busRaw = (((unsigned) Wire.read()) << 8) | ((unsigned) Wire.read());

            } while ((busRaw & 0x0002) == 0);       // Spin until CNVR goes high.

            accumRaw += (busRaw >> 3);      // Clobber OVF, CNVR, & reserved bit, and add to running sum.
                                            // At these voltages, the accumulator can't overflow.
        }
        *busV = (busScale * (accumRaw >> 3)) + busOffset;   // "Decimate" by shifting right n bits (see appnote)
    }
    if (currentMA != NULL) {
        Wire.beginTransmission(chipAddress);
        Wire.write(ConfigRegister);
        Wire.write(configShuntV >> 8);
        Wire.write(configShuntV & 0x00FF);
        Wire.endTransmission();                 // Trigger a new SHUNT voltage conversion.

        Wire.beginTransmission(chipAddress);    // It, too, is complete when the CNVR bit
        Wire.write(BusVoltageRegister);         // ...in the Bus Voltage Register goes high.
        Wire.endTransmission();

        do {
            Wire.requestFrom(chipAddress, 2);
            busRaw = (((unsigned) Wire.read()) << 8) | ((unsigned) Wire.read());
        } while ((busRaw & 0x0002) == 0);

        Wire.beginTransmission(chipAddress);    // Result is available in the Shunt Voltage Register
        Wire.write(ShuntVoltageRegister);
        Wire.endTransmission();
        Wire.requestFrom(chipAddress, 2);
        shuntRaw = (int)((((unsigned) Wire.read()) << 8) | ((unsigned) Wire.read()));

        currentTimestamp = millis();
        coulombs += shuntRaw * (currentTimestamp - previousTimestamp);   // Sum (I dt)
        previousTimestamp = currentTimestamp;

        *currentMA = (shuntScale * shuntRaw) + shuntOffset;

    }
}

float GetCoulombCount (void)    // Return coulomb counter value (in mAh)
{
    float mAms = (shuntScale * (float) coulombs) +
        (shuntOffset * (float) (currentTimestamp - firstTimestamp));
    return mAms / (1000.0 * 60.0 * 60.0);

}

void ResetCoulombCounter (void)    // Initialize coulomb counter variables
{
    coulombs = 0;
    firstTimestamp = previousTimestamp = currentTimestamp = millis();

}


void SetPGA (byte divisor)
{
    coList k = FetchConstants(chipAddress, divisor);
    shuntScale = k.shuntScale;
    shuntOffset = k.shuntOffset;

    switch (divisor) {
      case 1:
        configShuntV &= 0xE7FF;
        configShuntV |= 0x0000;
        break;
      case 2:
        configShuntV &= 0xE7FF;
        configShuntV |= 0x0800;
        break;
      case 4:
        configShuntV &= 0xE7FF;
        configShuntV |= 0x1000;
        break;
      case 8:
        configShuntV &= 0xE7FF;
        configShuntV |= 0x1800;
        break;
    }

}


byte GetPGA (void)
{
    switch (configShuntV & 0x1800) {
      case 0x0000:
        return 1;
      case 0x0800:
        return 2;
      case 0x1000:
        return 4;
      case 0x1800:
        return 8;
    }
}


static int GetINA219Address (void)
{
    int chipAddr;

    for (chipAddr = 0x40; chipAddr < 0x50; chipAddr++) {
        Wire.beginTransmission(chipAddr);
        Wire.write(ConfigRegister);
        Wire.endTransmission();
        delay(1);
        Wire.requestFrom(chipAddr, 2);
        if (Wire.available() == 2) {
            Wire.read();
            Wire.read();
            return chipAddr;
        }
    }
    return 0x00;
}


coList FetchConstants (byte chipAddress, byte pgaDivisor)
{
    coList k;

    switch (chipAddress) {
      case 0x41:                             // Mike's OSH Park PCB:
        k.busScale = 0.0005002685684646561;
        k.busOffset = 0.02374716782394622;         // 'Calib219V.nb', 11/21/15
        switch (pgaDivisor) {
//        case 1:
//          k.shuntScale = 0.09581313567602281;    // 'Calib219I.nb', 11/21/15
//          k.shuntOffset = 0.05481343136293849;
//          break;
//        case 2:
//          k.shuntScale = 0.0957912383713292;
//          k.shuntOffset = 0.12102293360328849;
//          break;
//        case 4:
//          k.shuntScale = 0.09574631628482281;
//          k.shuntOffset = 0.15934066397565305;
//          break;
          case 8:
            k.shuntScale = 0.09567442458722034;
            k.shuntOffset = 0.43657551106382464;
            break;
          default:
            k.shuntScale = 0.0;
            k.shuntOffset = 0.0;
            break;
        }
        break;

      case 0x44:                            // Hutch's version 4 stripboard:
        k.busScale = 0.000499663546363088;
        k.busOffset = 0.023826837671084893;       // 'Calib219V.nb', 03/22/15
        switch (pgaDivisor) {
//        case 1:
//          k.shuntScale = 0.0996220581947172;    // 'Calib219I8.nb', 03/24/15
//          k.shuntOffset = 0.02270981277080475;
//          break;
//        case 2:
//          k.shuntScale = 0.09958108326824884;
//          k.shuntOffset = -0.012986532466452427;
//          break;
//        case 4:
//          k.shuntScale = 0.09951684096712951;
//          k.shuntOffset = -0.11590949128942318;
//          break;
          case 8:
            k.shuntScale = 0.09941834338687703;
            k.shuntOffset = -0.16818951271813098;
            break;
          default:
            k.shuntScale = 0.0;
            k.shuntOffset = 0.0;
            break;
        }
        break;

      default:
        k.busScale = 0.0;
        k.busOffset = 0.0;
        k.shuntScale = 0.0;
        k.shuntOffset = 0.0;
        break;
    }
    return k;

}
