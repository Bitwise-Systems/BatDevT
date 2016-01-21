//---------------------------------------------------------------------------------
//    Watermark.ino -- Write a distinctive pattern across all of SRAM during
//                     system initialization. Provide the means to dump SRAM
//                     contents to the serial port.
//---------------------------------------------------------------------------------

#include <avr/io.h>

#define RamStart 0x100
#define RamEnd   0x900


//   The following "function" runs right after SREG, SPH, SPL, & r1 are
//   initialized, but before .ds variables are copied from flash and .bs
//   variables are set to zero. Interrupts are disabled at this point.
//   See 'AVR C Library', section 4.6, 'The .initN Sections'.

void watermark (void) __attribute__ ((naked)) __attribute__ ((section (".init3")));

void watermark (void)
{
    for (unsigned p = RamStart; p < RamEnd; p++)
        *(byte *)p = 0xaa;
}


//
//    DumpRam -- Print SRAM contents to the serial port in hexdump format
//

exitStatus DumpRAM (char **args)
{
    for (unsigned p = RamStart; p < RamEnd; p++) {
        if (p % 16 == 0) {
            Serial.print(F("\n"));
            dumpWord(p);
            Serial.print(F(": "));
        }
        if (p % 8 == 0)
            Serial.print(F("  "));

        dumpByte(*(byte *)p);
        delay(5);
        Serial.print(F(" "));
    }
    Serial.print(F("\n"));
    return Success;

}


static void dumpWord (word value)
{
    dumpNibble(value >> 12);
    dumpNibble(value >> 8);
    dumpNibble(value >> 4);
    dumpNibble(value);
}


static void dumpByte (byte value)
{
    dumpNibble(value >> 4);
    dumpNibble(value);
}


static void dumpNibble (byte value)
{
    static const char hextable[] = "0123456789ABCDEF";

    Serial.write(hextable[value & (byte) 0x0F]);

}

