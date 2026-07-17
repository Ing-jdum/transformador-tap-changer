#include "SerialProtocol.h"

static const char* faultName(SerialProtocol::FaultState f) {
    switch (f) {
        case SerialProtocol::FaultState::NONE: return "NONE";
        case SerialProtocol::FaultState::OVERCURRENT: return "OVERCURRENT";
        case SerialProtocol::FaultState::OVERVOLTAGE: return "OVERVOLTAGE";
    }
    return "UNKNOWN";
}

void SerialProtocol::begin(RelayBank &relays, PZEMManager &pzem, TapController &tap) {
    relays_ = &relays;
    pzem_ = &pzem;
    tap_ = &tap;
    lineLen_ = 0;
    lastStatusMs_ = millis();

    Serial.println(F("TapChanger ready. Commands: MODE?, MODE=AUTO|TEST, STATUS?, TAP=0|1|2, RELAY=idx,0|1, PZEM?"));
}

void SerialProtocol::update() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (lineLen_ > 0) {
                lineBuf_[lineLen_] = '\0';
                handleLine(lineBuf_);
                lineLen_ = 0;
            }
            continue;
        }

        if (lineLen_ < LINE_BUF_SIZE - 1) {
            lineBuf_[lineLen_++] = c;
        }
        // silently drop characters beyond buffer capacity rather than
        // overflow; the terminating newline still resets the buffer.
    }

    unsigned long now = millis();
    if (now - lastStatusMs_ >= STATUS_INTERVAL_MS) {
        lastStatusMs_ = now;
        printStatusLine();
    }
}

void SerialProtocol::handleLine(char *line) {
    // Strip any stray leading/trailing whitespace.
    while (*line == ' ') line++;

    if (strcmp(line, "MODE?") == 0) {
        Serial.print(F("MODE="));
        Serial.println(mode_ == ControllerMode::AUTO ? F("AUTO") : F("TEST"));
        return;
    }
    if (strncmp(line, "MODE=", 5) == 0) {
        cmdMode(line + 5);
        return;
    }
    if (strcmp(line, "STATUS?") == 0) {
        cmdStatusQuery();
        return;
    }
    if (strcmp(line, "RESTORE") == 0) {
        // Manual attempt to clear a latched fault and re-close breakers.
        // The actual safety checks are performed in cmdRestore().
        // Implemented below as cmdRestore to keep handleLine concise.
        // (We call cmdStatusQuery here to keep ordering similar.)
        // Fall through to command handler.
        // We'll handle it explicitly.
    }
    if (strncmp(line, "TAP=", 4) == 0) {
        cmdTap(line + 4);
        return;
    }
    if (strncmp(line, "RELAY=", 6) == 0) {
        cmdRelay(line + 6);
        return;
    }
    if (strcmp(line, "PZEM?") == 0) {
        cmdPzemQuery();
        return;
    }

    if (strcmp(line, "RESTORE") == 0) {
        // RESTORE handled here — try to clear fault and reclose breakers.
        // Delegate to a helper implemented below.
        // It needs access to relays_ and pzem_.
        // Implemented inline for minimal changes.
        const PZEMReading &g = pzem_->grid();
        if (fault_ == FaultState::OVERVOLTAGE) {
            if (!g.valid) {
                Serial.println(F("ERR cannot RESTORE: grid reading invalid"));
                return;
            }
            if (g.voltage < 230.0f) {
                relays_->setBreakers(true);
                clearFault();
                Serial.println(F("OK RESTORE"));
            } else {
                Serial.println(F("ERR cannot RESTORE: grid voltage >= 230V"));
            }
            return;
        } else if (fault_ == FaultState::OVERCURRENT) {
            if (!g.valid) {
                Serial.println(F("ERR cannot RESTORE: grid reading invalid"));
                return;
            }
            // Compute threshold same as protection logic in main
            if (g.voltage <= 0.0f) {
                Serial.println(F("ERR cannot RESTORE: invalid grid voltage"));
                return;
            }
            float ratedI = 300.0f / g.voltage;
            float thr = ratedI * 1.5f;
            if (g.current < thr) {
                relays_->setBreakers(true);
                clearFault();
                Serial.println(F("OK RESTORE"));
            } else {
                Serial.println(F("ERR cannot RESTORE: grid current still high"));
            }
            return;
        } else {
            Serial.println(F("ERR no active fault to RESTORE"));
            return;
        }
    }

    Serial.print(F("ERR unknown command: "));
    Serial.println(line);
}

void SerialProtocol::cmdMode(const char *arg) {
    if (strcmp(arg, "AUTO") == 0) {
        mode_ = ControllerMode::AUTO;
        Serial.println(F("OK MODE=AUTO"));
    } else if (strcmp(arg, "TEST") == 0) {
        mode_ = ControllerMode::TEST;
        Serial.println(F("OK MODE=TEST"));
    } else {
        Serial.println(F("ERR MODE must be AUTO or TEST"));
    }
}

void SerialProtocol::cmdTap(const char *arg) {
    if (mode_ != ControllerMode::TEST) {
        Serial.println(F("ERR TAP= only allowed in MODE=TEST"));
        return;
    }

    int idx = atoi(arg);
    if (idx < 0 || idx > 2) {
        Serial.println(F("ERR TAP must be 0, 1, or 2"));
        return;
    }

    Tap requested = static_cast<Tap>(idx);
    if (tap_->requestTap(requested)) {
        Serial.print(F("OK TAP="));
        Serial.println(idx);
    } else {
        Serial.println(F("ERR tap change rejected (already active or switch in progress)"));
    }
}

void SerialProtocol::cmdRelay(const char *arg) {
    if (mode_ != ControllerMode::TEST) {
        Serial.println(F("ERR RELAY= only allowed in MODE=TEST"));
        return;
    }

    char argCopy[16];
    strncpy(argCopy, arg, sizeof(argCopy) - 1);
    argCopy[sizeof(argCopy) - 1] = '\0';

    char *commaPos = strchr(argCopy, ',');
    if (!commaPos) {
        Serial.println(F("ERR RELAY format is RELAY=idx,0|1"));
        return;
    }
    *commaPos = '\0';

    int idx = atoi(argCopy);
    int val = atoi(commaPos + 1);

    if (idx < 0 || idx >= RelayBank::RAW_RELAY_COUNT) {
        Serial.println(F("ERR RELAY index must be 0-4"));
        return;
    }

    relays_->setRelayRaw(static_cast<uint8_t>(idx), val != 0);
    Serial.print(F("OK RELAY="));
    Serial.print(idx);
    Serial.print(',');
    Serial.println(val != 0 ? 1 : 0);
}

void SerialProtocol::cmdStatusQuery() {
    printStatusLine();
}

void SerialProtocol::cmdPzemQuery() {
    const PZEMReading &g = pzem_->grid();
    const PZEMReading &l = pzem_->load();

    Serial.print(F("PZEM GRID V="));
    Serial.print(g.voltage);
    Serial.print(F(" I="));
    Serial.print(g.current);
    Serial.print(F(" P="));
    Serial.print(g.power);
    Serial.print(F(" E="));
    Serial.print(g.energy);
    Serial.print(F(" F="));
    Serial.print(g.frequency);
    Serial.print(F(" PF="));
    Serial.print(g.powerFactor);
    Serial.print(F(" VALID="));
    Serial.println(g.valid ? 1 : 0);

    Serial.print(F("PZEM LOAD V="));
    Serial.print(l.voltage);
    Serial.print(F(" I="));
    Serial.print(l.current);
    Serial.print(F(" P="));
    Serial.print(l.power);
    Serial.print(F(" E="));
    Serial.print(l.energy);
    Serial.print(F(" F="));
    Serial.print(l.frequency);
    Serial.print(F(" PF="));
    Serial.print(l.powerFactor);
    Serial.print(F(" VALID="));
    Serial.println(l.valid ? 1 : 0);

    Serial.print(F("FAULT_STATE="));
    Serial.println(faultName(fault_));
}

void SerialProtocol::printStatusLine() {
    const PZEMReading &g = pzem_->grid();
    const PZEMReading &l = pzem_->load();

    Serial.print(F("STATUS MODE="));
    Serial.print(mode_ == ControllerMode::AUTO ? F("AUTO") : F("TEST"));
    Serial.print(F(" TAP="));
    Serial.print(static_cast<uint8_t>(tap_->currentTap()));
    Serial.print(F(" SWITCHING="));
    Serial.print(tap_->switching() ? 1 : 0);
    Serial.print(F(" BREAKERS="));
    Serial.print(relays_->breakersClosed() ? 1 : 0);
    Serial.print(F(" GRID[V="));
    Serial.print(g.voltage);
    Serial.print(F(",I="));
    Serial.print(g.current);
    Serial.print(F(",P="));
    Serial.print(g.power);
    Serial.print(F(",PF="));
    Serial.print(g.powerFactor);
    Serial.print(F(",F="));
    Serial.print(g.frequency);
    Serial.print(F(",E="));
    Serial.print(g.energy);
    Serial.print(F("]"));

    Serial.print(F(" LOAD[V="));
    Serial.print(l.voltage);
    Serial.print(F(",I="));
    Serial.print(l.current);
    Serial.print(F(",P="));
    Serial.print(l.power);
    Serial.print(F(",PF="));
    Serial.print(l.powerFactor);
    Serial.print(F(",F="));
    Serial.print(l.frequency);
    Serial.print(F(",E="));
    Serial.print(l.energy);
    Serial.print(F("]"));
    Serial.print(F(" VALIDGRID="));
    Serial.print(g.valid ? 1 : 0);
    Serial.print(F(" VALIDLOAD="));
    Serial.println(l.valid ? 1 : 0);
    Serial.print(F(" FAULT_STATE="));
    Serial.println(faultName(fault_));
}
