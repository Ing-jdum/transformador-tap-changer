#include "RelayBank.h"

const uint8_t RelayBank::pins_[RAW_RELAY_COUNT] = {
    PIN_BREAK_1, PIN_BREAK_2, PIN_TAP_LOW, PIN_TAP_MID, PIN_TAP_HIGH
};

void RelayBank::begin() {
    for (uint8_t i = 0; i < RAW_RELAY_COUNT; i++) {
        pinMode(pins_[i], OUTPUT);
    }
    // Start fully de-energized: breakers open, no tap selected, until
    // the caller explicitly sets a startup tap and closes the breakers.
    for (uint8_t i = 0; i < RAW_RELAY_COUNT; i++) {
        writeRelay(pins_[i], false);
        rawState_[i] = false;
    }
    breakersClosed_ = false;
}

void RelayBank::writeRelay(uint8_t pin, bool energized) {
    bool level = RELAY_ACTIVE_LOW ? !energized : energized;
    digitalWrite(pin, level ? HIGH : LOW);
}

uint8_t RelayBank::pinForTap(Tap tap) const {
    switch (tap) {
        case Tap::LOW_90V:  return PIN_TAP_LOW;
        case Tap::MID_110V: return PIN_TAP_MID;
        case Tap::HIGH_127V: return PIN_TAP_HIGH;
    }
    return PIN_TAP_MID; // unreachable, keeps compiler quiet
}

void RelayBank::setBreakers(bool closed) {
    writeRelay(PIN_BREAK_1, closed);
    writeRelay(PIN_BREAK_2, closed);
    rawState_[0] = closed;
    rawState_[1] = closed;
    breakersClosed_ = closed;
}

void RelayBank::setTap(Tap tap) {
    writeRelay(PIN_TAP_LOW,  tap == Tap::LOW_90V);
    writeRelay(PIN_TAP_MID,  tap == Tap::MID_110V);
    writeRelay(PIN_TAP_HIGH, tap == Tap::HIGH_127V);
    rawState_[2] = (tap == Tap::LOW_90V);
    rawState_[3] = (tap == Tap::MID_110V);
    rawState_[4] = (tap == Tap::HIGH_127V);
    currentTap_ = tap;
}

void RelayBank::setRelayRaw(uint8_t index, bool energized) {
    if (index >= RAW_RELAY_COUNT) return;
    writeRelay(pins_[index], energized);
    rawState_[index] = energized;

    // Keep the logical view consistent so STATUS reporting doesn't lie
    // after a raw TEST-mode poke.
    if (index == 0 || index == 1) {
        breakersClosed_ = rawState_[0] && rawState_[1];
    } else if (energized) {
        currentTap_ = static_cast<Tap>(index - 2);
    }
}

bool RelayBank::getRelayRaw(uint8_t index) const {
    if (index >= RAW_RELAY_COUNT) return false;
    return rawState_[index];
}
