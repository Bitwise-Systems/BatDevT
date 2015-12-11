//---------------------------------------------------------------------------------------
//    ConstantCurrent.ino -- Charge at a specified current level until battery
//                           reaches full capacity, or any of various charging
//                           anomalies occurs.
//
//---------------------------------------------------------------------------------------

exitStatus ConstantCurrent (float targetMA, unsigned durationM)
{
    exitStatus rc;
    float shuntMA, upperBound, lowerBound, upperTarget, lowerTarget;

    upperBound  = targetMA + BandPlus;
    lowerBound  = targetMA - BandMinus;
    upperTarget = targetMA + 0.05;
    lowerTarget = targetMA - 0.05;

    ActivateDetector(durationM);          // Initialize end-of-charge detector

    if ((rc = RampUp(lowerTarget)) != Success)    // Ramp up to target current
        return rc;

    while (true) {                                // Maintain constant current...
        if (rc = BailOutQ())
            return rc;

        if (rc = (exitStatus) FullyChargedQ())
            return rc;

        Monitor(&shuntMA, NULL);

        if (shuntMA < lowerBound)                     // nudge upwards
            do {
                if (NudgeVoltage(+1) == 0)
                    return UpperBound;
                Monitor(&shuntMA, NULL);
            } while (shuntMA < lowerTarget);

        else if (shuntMA > upperBound)                // nudge downwards
            do {
                if (NudgeVoltage(-1) == 0)
                    return LowerBound;
                Monitor(&shuntMA, NULL);
            } while (shuntMA > upperTarget);

    }

}


exitStatus RampUp (float targetMA)
{
    float shuntMA, busV;

    Monitor(&shuntMA, NULL);
    while (shuntMA < targetMA) {
        if (NudgeVoltage(+1) == 0)     // Single-step ramp-up
            return UpperBound;
        if (HasExpired(ReportTimer)) {
            Monitor(NULL, &busV);
            GenReport(typeRampUp, shuntMA, busV, millis());
        }
        Monitor(&shuntMA, NULL);
    }
    Monitor(NULL, &busV);
    GenReport(typeRampUp, shuntMA, busV, millis());
    return Success;

}


//---------------------------------------------------------------------------------------
//    Charge-complete detector package
//---------------------------------------------------------------------------------------

static int runLength;
static float previousMA;

void ActivateDetector (unsigned chargeTimeM)
{
    runLength = 0;
    previousMA = 10000.0;

    InitSmoothing(&gaussStructure, kernelBuffer);

    StartTimer(MaxChargeTimer, (chargeTimeM * 60.0));
    StartTimer(ArmDetectorTimer, (10 * 60.0));      // 10 minutes
    ResyncTimer(ReportTimer);

}


boolean FullyChargedQ (void)
{
    unsigned long timeStamp;
    float shuntMA, smoothedMA, busV, batteryTemp, ambientTemp;

    if (! HasExpired(ReportTimer))    // Check at 5-second intervals
        return false;

    timeStamp = millis();
    Monitor(&shuntMA, &busV);
    GetTemperatures(&batteryTemp, &ambientTemp);
    GenReport(typeCC, shuntMA, busV, timeStamp);

    if (! IsRunning(MaxChargeTimer))
        return MaxTime;

    if ((batteryTemp - ambientTemp) > ChargeTemp)
        return ChargeTempThreshold;

    if ((batteryTemp - ambientTemp) < 3.0)
        return false;

    if (IsRunning(ArmDetectorTimer))    // No detectors for 1st ten minutes
        return false;

    smoothedMA = Smooth(shuntMA, &gaussStructure);
    runLength = (smoothedMA > previousMA) ? runLength + 1 : 0;
    previousMA = smoothedMA;
    if (runLength > 12)
        return DipDetected;

    return false;

}
