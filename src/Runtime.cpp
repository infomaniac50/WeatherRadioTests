
#include <Arduino.h>
#include <SI4707.h>

#include "Runtime.h"

void Runtime::onTuneComplete() {
    // 0 STCINT
    // Seek/Tune Complete Interrupt.
    // 0 = Tune complete has not been triggered.
    // 1 = Tune complete has been triggered.
    Radio.getTuneStatus(INTACK);  //  Using INTACK clears STCINT, CHECK preserves it.
    Radio.sameFlush();             //  This should be done after any tune function.
    Serial.print(F("Channel: "));
    Serial.println(Radio.tuneStatus.channel);
    Serial.print(F("RSSI: "));
    Serial.println(Radio.tuneStatus.rssi);
    Serial.print(F("SNR: "));
    Serial.println(Radio.tuneStatus.snr);
    //intStatusCopy |= RSQINT;         //  We can force it to get rsqStatus on any tune.
}

void Runtime::onReceivedSignalQuality() {
    // STATUS Response Bit 3 RSQINT
    // Received Signal Quality Interrupt.
    // 0 = Received Signal Quality measurement has not been triggered.
    // 1 = Received Signal Quality measurement has been triggered.
    Radio.getSignalStatus(INTACK);
    // Serial.println(Radio.signalStatus, BIN);
    // Serial.println(Radio.rssi, BIN);
    // Serial.println(Radio.snr, BIN);
    // Serial.println(Radio.freqoff, BIN);
}

void Runtime::onSame() {
    Radio.getSameStatus(INTACK);
    struct SameStatus sameStatus = Radio.sameStatus;
    if (sameStatus.eomdet)
    {
        Radio.sameFlush();
        //  More application specific code could go here. (Mute audio, turn something on/off, etc.)
        return;
    }

    if (Radio.msgStatus & MSGAVL && (!(Radio.msgStatus & MSGUSD)))  // If a message is available and not already used,
        Radio.sameParse();                              // parse it.

    if (Radio.msgStatus & MSGPAR)
    {
        Radio.msgStatus &= ~MSGPAR;                       // Clear the parse status, so that we don't logging.print it again.
    }

    if (Radio.msgStatus & MSGPUR)  //  Signals that the third header has been received.
        Radio.sameFlush();
}

void Runtime::onAlerts() {
    Radio.getAlertStatus(INTACK);

    // serial->print(F("1050Hz Alert Tone:\n"));
    // serial->printf("%p %d\n", F("\tTone Present: "), Radio.alertStatus.tonePresent);
    // serial->printf("%p %d\n", F("\tAlert Off Interrupt: "), Radio.alertStatus.alertoff_int);
    // serial->printf("%p %d\n", F("\tAlert On Interrupt: "), Radio.alertStatus.alerton_int);
}

void Runtime::processInterrupts() {
    InterruptStatus localCopy;
    localCopy = Radio.interruptStatus;

    if (localCopy.tuneComplete) {
        onTuneComplete();
    }

    if (localCopy.rsq) {
        onReceivedSignalQuality();
    }

    if (localCopy.same) {
        onSame();
    }

    if (localCopy.asq) {
        onAlerts();
    }
}
