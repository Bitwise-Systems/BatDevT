//---------------------------------------------------------------------------------
//    Watermark.ino -- Write a distinctive pattern across all of SRAM during
//                     system initialization. Provide the means to dump SRAM
//                     contents to the serial port. Dump will be formatted by
//                     'BatDev.py'.
//---------------------------------------------------------------------------------

#include <avr/io.h>

#define RamStart 0x100
#define RamEnd   0x900


//   The following "function" runs about seven machine language instructions
//   after a system reset, right after SREG, SPH, SPL, & r1 are initialized,
//   but before .ds variables are copied from flash and .bss variables are
//   set to zero. Interrupts are disabled at this point.  See 'AVR C Library',
//   section 4.6, 'The .initN Sections'.

void watermark (void) __attribute__ ((naked)) __attribute__ ((section (".init3")));

void watermark (void)
{
    for (unsigned p = RamStart; p < RamEnd; p++)
        *(byte *)p = 0xaa;
}


//
//    DumpRam -- Print SRAM contents to the serial port as ASCII hex characters
//

exitStatus DumpRAM (char **args)
{
    unsigned p;

    Printf("$D");                              // Request formatting from BatDev.py
    for (p = RamStart; p < RamEnd; p++) {
        dumpByte(*(byte *) p);
        delay(5);
        Printf("%s", (p % 16 == 15) ? "\n" : " ");
    }
    return Success;

}


static void dumpByte (byte value)
{
    static const char hextable[] = "0123456789ABCDEF";

    Serial.write(hextable[(value >> 4) & (byte) 0x0F]);
    Serial.write(hextable[(value) & (byte) 0x0F]);

}

