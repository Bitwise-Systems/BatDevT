//
//    Savitzky.h  -- Constants, structures, & buffers required for smoothing filter
//


#define WindowSize 11

struct SavStruct {    // Stucture for managing Savitzky-Golay windows:
    float *window;        // Pointer to window buffer
    int index;            // Current position (index of most recent data item)
};

float dataWindow[WindowSize];
struct SavStruct savitskyStructure = {dataWindow, 0};
