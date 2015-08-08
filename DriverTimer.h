//=============================================================================
//      DriverTimer.h  --  Virtual timer definitions
//=============================================================================


void InitTimerTask(unsigned interval, void (*callback)());


//-----------------------------------------------------------------------------
//  One-shot timers:
//-----------------------------------------------------------------------------

typedef enum {
    MaxChargeTimer,     // Used in ConstantCurrent et. al. to limit charge time
    ArmDetectorTimer,   // Used to prevent premature arming of dip detector
    ExtensionTimer,     // Used temporarily to extend charging beyond dip detection
    ReboundTimer,       // Used to control rebound time in discharger

    NumOneShot          // Additional timer ID's go before this line
} OneShotTimerID;

volatile unsigned long oneShot[NumOneShot];


//-----------------------------------------------------------------------------
//  Free-running timers:
//-----------------------------------------------------------------------------

struct FreeRunningStruct {
    const unsigned char reload;
    unsigned char count;
    bool state;
};

typedef enum {
    ReportTimer,        // 5-second report timer
    PulseTimer,         // 1-second charging pulse timer

    NumFreeRunning
} FreeRunningTimerID;

volatile struct FreeRunningStruct freeRunning[] = {    // 0.1 second increments
    {50}, {10}
};

#define NumFreeRunning (sizeof freeRunning / sizeof(struct FreeRunningStruct))

#define Running 0       // Free-running timer states...
#define Expired 1
