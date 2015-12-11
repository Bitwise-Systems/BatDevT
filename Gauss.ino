//========================================================================================
//    Gauss.ino  --  Gaussian smoothing filter                              12-9-2015
//
//    Differences between this code & 'Savitzky.ino' (besides different kernel matrices):
//      1) Coefficient array moved to SRAM in the interest of speed,
//      2) No longer assuming rational kernel coefficients,
//      3) Coefficient array made local to 'Smooth',
//      4) Coefficients generated in Mathematica via the expression:
//           Reverse /@ Table[GaussianMatrix[{{n}}][[1 ;; n + 1]], {n, 2, 12}]
//      5) Trivial name changes to emphasize that this is a general-purpose,
//         convolution-based smoothing algorithm.
//
//========================================================================================


//----------------------------------------------------------------------------------------
//  InitSmoothing -- Initialize kernel structure in preparation for data smoothing
//----------------------------------------------------------------------------------------

void InitSmoothing (struct kernelStruct *s, float *kernelBuffer)
{
    (*s).index = 0;
    (*s).count = 1;
    (*s).window = kernelBuffer;

}


//----------------------------------------------------------------------------------------
//  Smooth -- Return the smoothed value associated with the most recent datum, x.
//            Requires a pointer to a structure that manages the kernel buffer.
//----------------------------------------------------------------------------------------

float Smooth (float x, struct kernelStruct *s)
{
    float sum = 0.0;

#if KernelSize == 5
  const static float c[3] = {
    0.47455888214414965,
    0.21183832321622456,
    0.05088223571170066
  };

#elif KernelSize == 7
  const static float c[4] = {
    0.29412786011227393,
    0.2161370497822301,
    0.102006038083625,
    0.034792982078007906
  };

#elif KernelSize == 9
  const static float c[5] = {
    0.21255267741127817,
    0.18354404297844593,
    0.12078065592205517,
    0.06276338705639073,
    0.026635575337469096
  };

#elif KernelSize == 11
  const static float c[6] = {
    0.16795251982137419,
    0.15385744484189667,
    0.1187181374719673,
    0.07787783685983768,
    0.04395541408652306,
    0.021614906829088183
  };

#elif KernelSize == 13
  const static float c[7] = {
    0.13925457976170105,
    0.13127388917596255,
    0.11008260438926494,
    0.08234828722517812,
    0.0551837462391462,
    0.033296068345937055,
    0.018188114743660593
  };

#elif KernelSize == 15
  const static float c[8] = {
    0.1190757010926205,
    0.11410692444451541,
    0.10044599914249554,
    0.08130823084696587,
    0.06062155954398166,
    0.04171864094069211,
    0.02656552612300851,
    0.015695268412030706
  };

#elif KernelSize == 17
  const static float c[9] = {
    0.10406640252193743,
    0.10075997932929888,
    0.09147140510577509,
    0.07789212805285486,
    0.062261857085954664,
    0.04676119950987791,
    0.03303610739228071,
    0.021984118965667376,
    0.01380000329732175
  };

#elif KernelSize == 19
  const static float c[10] = {
    0.0924479991124727,
    0.09013563966423373,
    0.08354571371353602,
    0.0736327826343995,
    0.06172859293297323,
    0.04924617801890385,
    0.03740949267672438,
    0.02707758976603025,
    0.018689183702678854,
    0.012310827334283847
  };

#elif KernelSize == 21
  const static float c[11] = {
    0.08318004430950358,
    0.08149909651541407,
    0.0766601165882705,
    0.06923347786129083,
    0.06004408190156071,
    0.05001937165279137,
    0.04003633324044414,
    0.030801931697378195,
    0.02278725148991233,
    0.016218090743834398,
    0.011110226154351624
  };

#elif KernelSize == 23
  const static float c[12] = {
    0.07561080756906689,
    0.07435035310775004,
    0.07069508174376112,
    0.06500224312510391,
    0.057802074842914046,
    0.0497157439930936,
    0.04136711815098226,
    0.033305647536505625,
    0.025952934167144993,
    0.019578475745619026,
    0.014302932070578244,
    0.010121991732013695
  };

#elif KernelSize == 25
  const static float c[13] = {
    0.06931029994056279,
    0.0683407776077056,
    0.06551359007346798,
    0.06106148982176469,
    0.05533667510317387,
    0.048764450909948284,
    0.041790994294854895,
    0.03483411947833,
    0.028244392275504362,
    0.022281056244772543,
    0.017103864153118094,
    0.012778909493040266,
    0.009294530574037998
  };

#else
  #error "Invalid KernelSize in Gauss.ino"

#endif

    (*s).index = ((*s).index + 1) % KernelSize;    // Slide the window to the right,
    (*s).window[(*s).index] = x;                   // ...and append the most recent data.

    if ((*s).count < KernelSize) {       // Return unsmoothed data while window fills up.
        (*s).count += 1;
        return x;
    } else {                                               // Run the algorithm...
        for (int i = 0; i < ((KernelSize + 1) / 2); i++)
            sum += c[i] * FetchPair(i, s);
        return sum;
    }

}


//----------------------------------------------------------------------------------------
//  FetchPair -- Return the sum of the window elements offset n cells to the left
//               and right of center.  If n == 0, just return the center element.
//----------------------------------------------------------------------------------------

static float FetchPair (byte n, struct kernelStruct *s)
{
    int left  = (*s).index - ((KernelSize-1) / 2) - n;    // Compute offsets from center.
    int right = (*s).index - ((KernelSize-1) / 2) + n;

    if (left < 0)   left += KernelSize;        // Deal with circular buffer wrap issues.
    if (right < 0)  right += KernelSize;

    if (n == 0)
        return (*s).window[left];              // Or right. Doesn't matter: they're equal.
    else
        return (*s).window[left] + (*s).window[right];

}
