//
//    Savitzky.h  -- Constants, structures, & buffers required for smoothing filter
//
//    Version 2: Window contents can be flushed & buffering restarted
//


#define WindowSize 11

struct SavStruct {    // Stucture for managing Savitzky-Golay windows:
    float *window;        // Pointer to window buffer
    byte index;           // Current position (index of most recent data item)
    byte count;           // Count of currently occupied window slots (+1)
};

float dataWindow[WindowSize];
struct SavStruct savitskyStructure;
