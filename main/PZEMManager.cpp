#include "PZEMManager.h"

void PZEMManager::begin() {
    PZEM_GRID_SERIAL.begin(PZEM_BAUD);
    PZEM_LOAD_SERIAL.begin(PZEM_BAUD);
}

PZEMReading PZEMManager::readSensor(PZEM004Tv30 &pzem) {
    PZEMReading r;
    r.voltage = pzem.voltage();

    // A NaN voltage means the Modbus read failed — don't trust any of
    // the other fields from this cycle.
    if (isnan(r.voltage)) {
        r.valid = false;
        return r;
    }

    r.current     = pzem.current();
    r.power       = pzem.power();
    r.energy      = pzem.energy();
    r.frequency   = pzem.frequency();
    r.powerFactor = pzem.pf();
    r.valid = true;
    return r;
}

void PZEMManager::update() {
    unsigned long now = millis();
    if (now - lastReadMs_ < PZEM_READ_INTERVAL_MS) return;
    lastReadMs_ = now;

    gridReading_ = readSensor(pzemGrid_);
    loadReading_ = readSensor(pzemLoad_);
}
