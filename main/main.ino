// =============================================================
// Automatic autotransformer tap changer — Arduino Mega
//
// AUTO mode: load-side voltage (PZEM on Serial3) drives tap selection
// through TapController's hysteresis + confirm-timer state machine.
//
// TEST mode: raw relay control and manual tap requests over serial,
// for bench verification of relays and both PZEM sensors before
// trusting the system to run unattended.
//
// See SerialProtocol.h for the full command/status wire format used
// by the future Kotlin Multiplatform UI.
// =============================================================

#include "Config.h"
#include "RelayBank.h"
#include "PZEMManager.h"
#include "TapController.h"
#include "SerialProtocol.h"

RelayBank relays;
PZEMManager pzem;
TapController tapController;
SerialProtocol protocol;

void setup() {
    Serial.begin(SERIAL_BAUD);

    relays.begin();
    pzem.begin();
    tapController.begin(relays);
    protocol.begin(relays, pzem, tapController);
}

void loop() {
    pzem.update();
    tapController.update(pzem.load().voltage, protocol.mode());
    protocol.update();
}
