//
//    Smoothing.h  -- Constants, structures, & buffers required for smoothing filter
//

#define KernelSize 11    // Currently, the coefficient array 'c' hardcodes this

struct kernelStruct {    // Stucture for managing convolution kernels:
    float *window;       //     pointer to window buffer,
    byte index;          //     current position (index of most recent data item),
    byte count;          //     count of currently occupied window slots (+1)
};

float kernelBuffer[KernelSize];
struct kernelStruct gaussStructure;
