#pragma once
#include <Arduino.h>
#include "Config.h"

// Low-level relay driver. Knows nothing about hysteresis, timing, or
// switching sequences — TapController owns that logic. RelayBank's only
// job is to guarantee the physical pin states match the requested
// logical states, correctly honoring active-low wiring.
class RelayBank {
public:
    void begin();

    // true = line connected through to the load, false = isolated/open.
    void setBreakers(bool closed);
    bool breakersClosed() const { return breakersClosed_; }

    // Energizes exactly one tap relay, de-energizes the other two.
    void setTap(Tap tap);
    Tap currentTap() const { return currentTap_; }

    // Direct single-relay control for TEST mode only — bypasses all
    // interlocking. Index 0-4 maps to BREAK1, BREAK2, TAP_LOW, TAP_MID,
    // TAP_HIGH in that order.
    static constexpr uint8_t RAW_RELAY_COUNT = 5;
    void setRelayRaw(uint8_t index, bool energized);
    bool getRelayRaw(uint8_t index) const;

private:
    void writeRelay(uint8_t pin, bool energized);
    uint8_t pinForTap(Tap tap) const;

    bool breakersClosed_ = false;
    Tap currentTap_ = STARTUP_TAP;
    bool rawState_[RAW_RELAY_COUNT] = {false, false, false, false, false};

    static const uint8_t pins_[RAW_RELAY_COUNT];
};
