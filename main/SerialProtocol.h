#pragma once
#include <Arduino.h>
#include "Config.h"
#include "RelayBank.h"
#include "PZEMManager.h"
#include "TapController.h"

// -----------------------------------------------------------------------
// Wire protocol (host <-> device), line-based ASCII, newline-terminated.
// Designed to be trivial to parse from the future Kotlin Multiplatform UI
// without a JSON library on the Mega side.
//
// Host -> device commands:
//   MODE?                 -> reply MODE=AUTO or MODE=TEST
//   MODE=AUTO | MODE=TEST -> switch mode
//   STATUS?                -> single-line status dump (see below)
//   TAP=0|1|2              -> TEST mode only: request tap via the safe
//                              break-before-make sequence
//   RELAY=<idx>,<0|1>      -> TEST mode only: raw relay control,
//                              idx 0=BREAK1 1=BREAK2 2=TAP_LOW 3=TAP_MID 4=TAP_HIGH
//   PZEM?                   -> print grid + load readings
//
// Device -> host:
//   Unsolicited status line every STATUS_INTERVAL_MS, plus in response
//   to STATUS?:
//     STATUS MODE=<AUTO|TEST> TAP=<0|1|2> SWITCHING=<0|1> BREAKERS=<0|1>
//            VGRID=<f> IGRID=<f> VLOAD=<f> ILOAD=<f> VALIDGRID=<0|1> VALIDLOAD=<0|1>
//   Errors: ERR <reason>
//   Acks:   OK <echo of applied command>
// -----------------------------------------------------------------------
class SerialProtocol {
public:
    void begin(RelayBank &relays, PZEMManager &pzem, TapController &tap);

    // Call every loop(). Reads any pending serial input and emits the
    // periodic status broadcast.
    void update();

    ControllerMode mode() const { return mode_; }

    enum class FaultState : uint8_t { NONE = 0, OVERCURRENT = 1, OVERVOLTAGE = 2 };

    void setFault(FaultState f) { fault_ = f; }
    FaultState fault() const { return fault_; }
    void clearFault() { fault_ = FaultState::NONE; }

private:
    static constexpr uint8_t LINE_BUF_SIZE = 48;

    void handleLine(char *line);
    void cmdMode(const char *arg);
    void cmdTap(const char *arg);
    void cmdRelay(const char *arg);
    void cmdStatusQuery();
    void cmdPzemQuery();
    void printStatusLine();

    RelayBank *relays_ = nullptr;
    PZEMManager *pzem_ = nullptr;
    TapController *tap_ = nullptr;

    ControllerMode mode_ = ControllerMode::AUTO;

    char lineBuf_[LINE_BUF_SIZE];
    uint8_t lineLen_ = 0;

    unsigned long lastStatusMs_ = 0;
    FaultState fault_ = FaultState::NONE;
};
