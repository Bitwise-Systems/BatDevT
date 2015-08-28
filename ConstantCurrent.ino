//---------------------------------------------------------------------------------------
//    ConstantCurrent.ino -- Charge at a specified current level until battery
//                           reaches full capacity, a time limit is exceeded, or
//                           any of various charging anomalies occurs.
//
//    . Put bail-out tests into a separate routine.
//    . Removed nudge testing code.
//    . Switched timer management over to virtual timer package.
//    . Ramp-up now occurs separately from maintenance loop.
//
//---------------------------------------------------------------------------------------


exitStatus ConstantCurrent (float targetMA, unsigned durationM, float maxV)
{
    exitStatus bailRC;
    unsigned long timeStamp;
    float shuntMA, busV, batteryTemp, ambientTemp;
    float upperBound, lowerBound, upperTarget, lowerTarget;

    upperBound  = targetMA + bandPlus;
    lowerBound  = targetMA - bandMinus;
    upperTarget = targetMA + 0.05;
    lowerTarget = targetMA - 0.05;

    ActivateDetector();
    ResyncTimer(ReportTimer);
    StartTimer(MaxChargeTimer, (durationM * 60.0));

    while (IsRunning(MaxChargeTimer)) {     // Single-step ramp-up
        timeStamp = millis();
        Monitor(&shuntMA, &busV);
        GetTemperatures(&batteryTemp, &ambientTemp);
        bailRC = BailOutQ(busV, batteryTemp);

        if (bailRC != 0)
            return bailRC;

        else if (shuntMA > lowerTarget) {
            CTReport(666, shuntMA, busV, batteryTemp, ambientTemp, timeStamp);
            break;
        }
        else if (NudgeVoltage(+1) == 0)
            return BoundsCheck;

        if (HasExpired(ReportTimer))
            CTReport(666, shuntMA, busV, batteryTemp, ambientTemp, timeStamp);

    }   // Drop thru only if we ramped up to 'lowerTarget', or ran out of time trying

    while (IsRunning(MaxChargeTimer)) {     // Maintain constant current
        timeStamp = millis();
        Monitor(&shuntMA, &busV);
        GetTemperatures(&batteryTemp, &ambientTemp);

        bailRC = BailOutQ(busV, batteryTemp);
        if (bailRC != 0)
            return bailRC;

        if (shuntMA < lowerBound)        // nudge upwards
            do {
                if (NudgeVoltage(+1) == 0)
                    return BoundsCheck;
                Monitor(&shuntMA, NULL);
            } while (shuntMA < lowerTarget);

        else if (shuntMA > upperBound)   // nudge downwards
            do {
                if (NudgeVoltage(-1) == 0)
                    return BoundsCheck;
                Monitor(&shuntMA, NULL);
            } while (shuntMA > upperTarget);

        if (HasExpired(ReportTimer)) {
            CTReport(typeCCRecord, shuntMA, busV, batteryTemp, ambientTemp, timeStamp);
            if (FullyCharged(shuntMA, (batteryTemp - ambientTemp)))
                return Success;
        }
    }
    return MaxTime;

}


//---------------------------------------------------------------------------------------
//    Charge-complete detector package
//---------------------------------------------------------------------------------------

int runLength;
float previousMA;
boolean extending;        // <<< Temporary: remove when time extension nolonger needed.

void ActivateDetector (void)
{
    StartTimer(ArmDetectorTimer, (10 * 60.0));    // 10 minutes
    runLength = 0;
    previousMA = 10000.0;
    extending = false;

}


boolean FullyCharged (float shuntMA, float tempDifferential)
{
    float smoothedMA;

    if (extending)
        return (!IsRunning(ExtensionTimer));

    if (IsRunning(ArmDetectorTimer))    // No detectors for 1st ten minutes
        return false;

    if (tempDifferential < 3.0)
        return false;

    smoothedMA = Savitzky(shuntMA, &savitskyStructure);
    runLength = (smoothedMA > previousMA) ? runLength + 1 : 0;
    previousMA = smoothedMA;
    if (runLength > 12) {
        CTReport(2, shuntMA, 0.0, tempDifferential, 0.0, millis());
    //  return true;
        extending = true;                              // <<< Temporary: extend run...
        StartTimer(ExtensionTimer, 10 * 60.0);         // <<< ...for ten more minutes
    }
    return false;

}


//---------------------------------------------------------------------------------------
//    Alternative current trend detector:
//
//    runLength = (shuntMA < previousMA) ? 0 : runLength + 1;
//    previousMA = shuntMA;
//    if (runLength > 5)
//        return true;
//
//---------------------------------------------------------------------------------------

