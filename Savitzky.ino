//----------------------------------------------------------------------------------------
//    Savitzky.ino  --  Savitzky-Golay smoothing filter
//
//    Version 2: Window contents can be flushed & buffering restarted
//
//----------------------------------------------------------------------------------------

#include <avr/pgmspace.h>

//  Savitzky-Golay coefficients for window sizes 5--25.

#if WindowSize == 5
  const float PROGMEM h = 35.0;
  const float PROGMEM c[] = {17.0, 12.0, -3.0};

#elif WindowSize == 7
  const float PROGMEM h = 21.0;
  const float PROGMEM c[] = {7.0, 6.0, 3.0, -2.0};

#elif WindowSize == 9
  const float PROGMEM h = 231.0;
  const float PROGMEM c[] = {59.0, 54.0, 39.0, 14.0, -21.0};

#elif WindowSize == 11
  const float PROGMEM h = 429.0;
  const float PROGMEM c[] = {89.0, 84.0, 69.0, 44.0, 9.0, -36.0};

#elif WindowSize == 13
  const float PROGMEM h = 143.0;
  const float PROGMEM c[] = {25.0, 24.0, 21.0, 16.0, 9.0, 0.0, -11.0};

#elif WindowSize == 15
  const float PROGMEM h = 1105.0;
  const float PROGMEM c[] = {167.0, 162.0, 147.0, 122.0, 87.0, 42.0, -13.0, -78.0};

#elif WindowSize == 17
  const float PROGMEM h = 323.0;
  const float PROGMEM c[] = {43.0, 42.0, 39.0, 34.0, 27.0, 18.0, 7.0, -6.0, -21.0};

#elif WindowSize == 19
  const float PROGMEM h = 2261.0;
  const float PROGMEM c[] = {269.0, 264.0, 249.0, 224.0, 189.0, 144.0, 89.0, 24.0, -51.0, -136.0};

#elif WindowSize == 21
  const float PROGMEM h = 3059.0;
  const float PROGMEM c[] = {329.0, 324.0, 309.0, 284.0, 249.0, 204.0, 149.0, 84.0, 9.0, -76.0, -171.0};

#elif WindowSize == 23
  const float PROGMEM h = 805.0;
  const float PROGMEM c[] = {79.0, 78.0, 75.0, 70.0, 63.0, 54.0, 43.0, 30.0, 15.0, -2.0, -21.0, -42.0};

#elif WindowSize == 25
  const float PROGMEM h = 5175.0;
  const float PROGMEM c[] = {467.0, 462.0, 447.0, 422.0, 387.0, 343.0, 287.0, 222.0, 147.0, 62.0, -33.0, -138.0, -253.0};

#else
  #error "Invalid WindowSize in Savitzky-Golay algorithm."

#endif


//----------------------------------------------------------------------------------------
//  InitSavitzky --
//----------------------------------------------------------------------------------------

void InitSavitzky (struct SavStruct *s, float *windowBuffer)
{
    (*s).index = 0;
    (*s).count = 1;
    (*s).window = windowBuffer;

}


//----------------------------------------------------------------------------------------
//  Savitzky -- Return the "smoothed" value associated with the most recent data, x.
//              Requires a pointer to a structure that manages the window buffer (and
//              eventually other matters).
//----------------------------------------------------------------------------------------

float Savitzky (float x, struct SavStruct *s)
{
    int j;
    float k, sum = 0.0;

    (*s).index = ((*s).index + 1) % WindowSize;    // Slide the window to the right, ...
    (*s).window[(*s).index] = x;                   // ...and append the most recent data.

    if ((*s).count < WindowSize) {    // Return unsmoothed data while window fills up.
        (*s).count += 1;
        return x;
    } else {
        for (j = 0; j < ((WindowSize+1) / 2); j++) {   // Run the algorithm...
            k = pgm_read_float_near(&c[j]);
            sum += k * FetchPair(j, s);
        }
        return (sum / pgm_read_float_near(&h));
    }

}


//----------------------------------------------------------------------------------------
//  FetchPair -- Return the sum of the window elements offset n cells to the left
//               and right of center.  If n == 0, just return the center element.
//----------------------------------------------------------------------------------------

static float FetchPair (byte n, struct SavStruct *s)
{
    int left  = (*s).index - ((WindowSize-1) / 2) - n;    // Compute offsets from center.
    int right = (*s).index - ((WindowSize-1) / 2) + n;

    if (left < 0)   left += WindowSize;        // Deal with circular buffer wrap issues.
    if (right < 0)  right += WindowSize;

    if (n == 0)
        return (*s).window[left];              // Or right. Doesn't matter: they're equal.
    else
        return (*s).window[left] + (*s).window[right];

}
