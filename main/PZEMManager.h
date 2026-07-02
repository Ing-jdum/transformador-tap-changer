#pragma once
#include <PZEM004Tv30.h>
#include "Config.h"

struct PZEMReading {
    float voltage = NAN;
    float current = NAN;
    float power = NAN;
    float energy = NAN;
    float frequency = NAN;
    float powerFactor = NAN;
    bool valid = false;
};

// Wraps the two PZEM-004T sensors and caches the last reading of each,
// refreshed no more often than PZEM_READ_INTERVAL_MS so the control loop
// never blocks waiting on the Modbus round trip.
class PZEMManager {
public:
    void begin();
    void update(); // call every loop(); internally rate-limited

    const PZEMReading& grid() const { return gridReading_; }
    const PZEMReading& load() const { return loadReading_; }

private:
    static PZEMReading readSensor(PZEM004Tv30 &pzem);

    PZEM004Tv30 pzemGrid_{PZEM_GRID_SERIAL};
    PZEM004Tv30 pzemLoad_{PZEM_LOAD_SERIAL};

    PZEMReading gridReading_;
    PZEMReading loadReading_;
    unsigned long lastReadMs_ = 0;
};
