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
    // Protection: automatic breaker trip on primary overcurrent or overvoltage.
    const PZEMReading &g = pzem.grid();
    // Only evaluate when we have a valid grid reading.
    if (g.valid) {
        // Overvoltage: trip if grid voltage strictly exceeds 230V.
        if (g.voltage > 230.0f && protocol.fault() == SerialProtocol::FaultState::NONE) {
            relays.setBreakers(false);
            protocol.setFault(SerialProtocol::FaultState::OVERVOLTAGE);
        }

        // Overcurrent: compute rated current from 300W and apply 1.5x margin.
        if (g.voltage > 0.0f && protocol.fault() == SerialProtocol::FaultState::NONE) {
            float ratedI = 300.0f / g.voltage;
            float thr = ratedI * 1.5f;
            if (g.current > thr) {
                relays.setBreakers(false);
                protocol.setFault(SerialProtocol::FaultState::OVERCURRENT);
            }
        }
    }
    tapController.update(pzem.load().voltage, protocol.mode());
    protocol.update();
}
