#pragma once
#include <Arduino.h>
#include "Config.h"
#include "RelayBank.h"

enum class ControllerMode : uint8_t { AUTO, TEST };

// Owns the tap-switching state machine. In AUTO mode it watches the
// load-side voltage and steps between taps using per-boundary hysteresis
// plus a confirm timer that rejects short transients. Every switch —
// automatic or a manual TEST-mode request — goes through the same
// non-blocking break-before-make sequence, so the tap-select relays
// never operate under load.
class TapController {
public:
    void begin(RelayBank &relays);

    // Call every loop(). In AUTO mode, pass the latest load voltage
    // (NAN if the last PZEM read was invalid — the controller holds
    // its current tap rather than guess). Ignored while a switch is
    // already in progress.
    void update(float loadVoltage, ControllerMode mode);

    // Explicit tap request — used by TEST mode for manual verification.
    // Returns false if a switch is already in progress or the tap is
    // already active.
    bool requestTap(Tap tap);

    Tap currentTap() const { return relays_->currentTap(); }
    bool switching() const { return state_ != State::IDLE; }

private:
    enum class State : uint8_t { IDLE, OPENING, SELECTING, CLOSING };

    static Tap decideTap(Tap current, float loadVoltage);
    void beginSwitch(Tap target);
    void runStateMachine();

    RelayBank *relays_ = nullptr;
    State state_ = State::IDLE;
    Tap pendingTap_ = STARTUP_TAP;
    unsigned long stateEnteredMs_ = 0;

    // Confirm-timer bookkeeping for hysteresis.
    Tap candidateTap_ = STARTUP_TAP;
    unsigned long candidateSinceMs_ = 0;
};
