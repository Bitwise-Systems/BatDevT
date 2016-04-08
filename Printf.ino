//
//    Printf.ino -- Simple Arduino implementation of 'printf'
//
//      Version 2: Format strings reside in program memory instead of SRAM
//
//      Limitations:
//          1) Provides a subset of the official conversion specifiers. In particular,
//             in order to keep the code footprint to a minimum, '%x' is missing (but
//             would be very easy to add).
//          2) Floating point format is very restrictive. Must be of the form: '%1.nf',
//             where n = [0-9]. Don't forget the '1'.
//          3) No support for precision modifiers, other than what's mentioned in (2).
//             The long modifier is presumed to apply just to integral types.
//             No flags or minimum field width support at all.
//          4) Unsigned longs are treated just like longs. Again, this is in the interest
//             of brevity. Beware of N > 2 billion.
//
//      Version 2.1: Support for '%x'. Shoulda just done it from the outset.
//
//

#include <stdarg.h>

void _Printf (const char *format, ...)
{
    long lval;
    float fval;
    va_list argp;
    unsigned uval;
    int ival, precision;
    char c, cval, *sval;

    va_start(argp, format);

    for (c = pgm_read_byte(format); c != '\0'; c = pgm_read_byte(++format)) {
        if (c != '%') {
            Serial.write(c);
            continue;
        }
        switch (c = pgm_read_byte(++format)) {
        case 'c':
            cval = va_arg(argp, int);
            Serial.write(cval);
            break;
        case 's':
            sval = va_arg(argp, char *);
            Serial.print(sval);
            break;
        case 'd':
            ival = va_arg(argp, int);
            Serial.print(ival);
            break;
        case 'u':
            uval = va_arg(argp, unsigned);
            Serial.print(uval);
            break;
        case 'l':
            format += 1;
            lval = va_arg(argp, long);
            Serial.print(lval);
            break;
        case '1':
            precision = pgm_read_byte(format + 2) - 0x30;
            format += 3;
            fval = va_arg(argp, double);
            Serial.print(fval, precision);
            break;
        case 'x':
            uval = va_arg(argp, unsigned);
            Serial.print(uval, HEX);
            break;
        default:
            Serial.write(c);
            break;
        }
    }
    va_end(argp);

}

