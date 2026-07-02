#include "TapController.h"

void TapController::begin(RelayBank &relays) {
    relays_ = &relays;
    relays_->setTap(STARTUP_TAP);
    relays_->setBreakers(true);
    candidateTap_ = STARTUP_TAP;
    candidateSinceMs_ = millis();
}

Tap TapController::decideTap(Tap current, float loadVoltage) {
    if (isnan(loadVoltage)) return current; // no valid reading: hold position

    switch (current) {
        case Tap::MID_110V:
            if (loadVoltage < MID_LOWER_BOUND) return Tap::HIGH_127V; // undershoot -> boost
            if (loadVoltage > MID_UPPER_BOUND) return Tap::LOW_90V;   // overshoot  -> buck
            return Tap::MID_110V;

        case Tap::LOW_90V: // engaged to buck a surge; ease back once output undershoots
            return (loadVoltage < LOW_EXIT_BOUND) ? Tap::MID_110V : Tap::LOW_90V;

        case Tap::HIGH_127V: // engaged to boost a sag; ease back once output overshoots
            return (loadVoltage > HIGH_EXIT_BOUND) ? Tap::MID_110V : Tap::HIGH_127V;
    }
    return current; // unreachable
}

bool TapController::requestTap(Tap tap) {
    if (state_ != State::IDLE) return false;
    if (tap == relays_->currentTap()) return false;
    beginSwitch(tap);
    return true;
}

void TapController::beginSwitch(Tap target) {
    pendingTap_ = target;
    state_ = State::OPENING;
    stateEnteredMs_ = millis();
    relays_->setBreakers(false);
}

void TapController::runStateMachine() {
    if (state_ == State::IDLE) return;

    unsigned long elapsed = millis() - stateEnteredMs_;
    if (elapsed < BREAK_DEAD_TIME_MS) return;

    switch (state_) {
        case State::OPENING:
            relays_->setTap(pendingTap_);
            state_ = State::SELECTING;
            stateEnteredMs_ = millis();
            break;

        case State::SELECTING:
            relays_->setBreakers(true);
            state_ = State::CLOSING;
            stateEnteredMs_ = millis();
            break;

        case State::CLOSING:
            state_ = State::IDLE;
            // Restart the confirm window fresh on the tap we just landed on.
            candidateTap_ = relays_->currentTap();
            candidateSinceMs_ = millis();
            break;

        case State::IDLE:
            break;
    }
}

void TapController::update(float loadVoltage, ControllerMode mode) {
    runStateMachine(); // always service an in-progress switch first

    if (mode != ControllerMode::AUTO || state_ != State::IDLE) return;

    Tap desired = decideTap(relays_->currentTap(), loadVoltage);

    if (desired == relays_->currentTap()) {
        candidateTap_ = desired; // nothing pending; keep timer reset
        return;
    }

    if (desired != candidateTap_) {
        candidateTap_ = desired;    // new candidate: (re)start confirm timer
        candidateSinceMs_ = millis();
        return;
    }

    if (millis() - candidateSinceMs_ >= CONFIRM_TIME_MS) {
        beginSwitch(desired);
    }
}