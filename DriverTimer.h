//=============================================================================
//      DriverTimer.h  --  Virtual timer definitions
//=============================================================================


void InitTimerTask(void (*callback)());


//-----------------------------------------------------------------------------
//  One-shot timers:
//-----------------------------------------------------------------------------

typedef enum {
    MaxChargeTimer,     // Used in ConstantCurrent et. al. to limit charge time
    ArmDetectorTimer,   // Used to prevent premature arming of dip detector
    ReboundTimer,       // Used to control rebound time in discharger
    OneShotTestTimer,   // variable-period timer for testing

    NumOneShot          // Additional timer ID's go before this line
} OneShotTimerID;

volatile unsigned long oneShot[NumOneShot];


//-----------------------------------------------------------------------------
//  Free-running timers:
//-----------------------------------------------------------------------------

#define Seconds(s) ((unsigned)(100.0 * (s)))    // convert seconds to 10ms ticks

struct FreeRunningStruct {
    const unsigned reload;
    unsigned count;
    bool state;
};

typedef enum {
    ReportTimer,        // 5-second report timer

    NumFreeRunning
} FreeRunningTimerID;


volatile struct FreeRunningStruct freeRunning[] = {
    { Seconds(5.0) }, { Seconds(1.0) }, { Seconds(12.0) }
};

#define NumFreeRunning (sizeof freeRunning / sizeof(struct FreeRunningStruct))

#define Running 0       // Free-running timer states...
#define Expired 1
