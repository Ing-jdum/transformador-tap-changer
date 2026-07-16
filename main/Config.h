#pragma once
#include <Arduino.h>

// =============================================================
// Hardware pin map
// =============================================================

// Break relays: isolate the autotransformer line from the load
// before the tap-select relay is changed. Both operate together.
constexpr uint8_t PIN_BREAK_1 = 2;
constexpr uint8_t PIN_BREAK_2 = 4;

// Tap-select relays: exactly one is energized at a time.
constexpr uint8_t PIN_TAP_LOW  = 5;  // ~90V tap
constexpr uint8_t PIN_TAP_MID  = 6;  // ~110V tap (startup default)
constexpr uint8_t PIN_TAP_HIGH = 7;  // ~127V tap

// Most opto-isolated relay boards (Elegoo etc.) pull the input LOW to energize.
constexpr bool RELAY_ACTIVE_LOW = true;

// =============================================================
// PZEM-004T v3.0 serial assignment
// =============================================================
// Serial2 (TX2=16, RX2=17): grid/input side (~220V) — telemetry only.
// Serial3 (TX3=14, RX3=15): load/output side — drives tap decisions.
#define PZEM_GRID_SERIAL  Serial3
#define PZEM_LOAD_SERIAL  Serial2
constexpr unsigned long PZEM_BAUD = 9600;

// =============================================================
// Tap identifiers
// =============================================================
enum class Tap : uint8_t { LOW_90V = 0, MID_110V = 1, HIGH_127V = 2 };

constexpr Tap STARTUP_TAP = Tap::MID_110V;

// =============================================================
// Voltage thresholds — hysteresis bands on LOAD-side voltage
// =============================================================
// This is a boost/buck stabilizer: for a given input, HIGH_127V produces
// the most output and LOW_90V the least. So the correction direction is
// the opposite of the tap's nominal label — a LOW load reading means the
// output needs MORE boost (move toward HIGH), not less.
//
// MID is the resting tap. It moves to HIGH when undershooting (needs
// boost) and to LOW when overshooting (needs buck). Once on an end tap,
// it unwinds back to MID only after it has over-corrected past that
// tap's own exit bound — never jumps directly LOW<->HIGH, always via MID.
constexpr float MID_LOWER_BOUND = 95.0f;    // MID -> HIGH (boost)
constexpr float MID_UPPER_BOUND = 113.5f;   // MID -> LOW  (buck)

// Return thresholds (provide hysteresis)
constexpr float LOW_EXIT_BOUND  = 93.0f;    // LOW  -> MID
constexpr float HIGH_EXIT_BOUND = 115.5f;   // HIGH -> MID

// =============================================================
// Timing
// =============================================================
// A threshold crossing must persist this long before a switch is triggered.
constexpr unsigned long CONFIRM_TIME_MS = 5000;

// Dead time held at each stage of the break-before-make sequence
// (breakers open -> tap relay changes -> breakers close).
constexpr unsigned long BREAK_DEAD_TIME_MS = 150;

constexpr unsigned long PZEM_READ_INTERVAL_MS = 1000;
constexpr unsigned long STATUS_INTERVAL_MS    = 1000;

constexpr unsigned long SERIAL_BAUD = 115200;