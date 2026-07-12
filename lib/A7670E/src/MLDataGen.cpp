#include "MLDataGen.h"
#include <string.h>
#include <stdio.h>

MLDataGen::MLDataGen(TelecomAnalyzer& analyzer,
                                    GPS& gnss,
                                    Logger& logger)
    : _analyzer(analyzer), _gnss(gnss), _logger(logger) {}

bool MLDataGen::collectSample(MLSample& sample, const char* label,
                                      float confidence, uint32_t gpsTimeoutMs) {
    memset(&sample, 0, sizeof(sample));

    // Cell data
    CellRecord cell;
    if (!_analyzer.sampleServingCell(cell)) return false;

    sample.rssi         = cell.rssi;
    sample.rsrp         = cell.rsrp;
    sample.rsrq         = cell.rsrq;
    sample.sinr         = cell.sinr;
    sample.csq          = cell.csq;
    sample.cellId       = cell.cellId;
    sample.lac          = cell.lac;
    strncpy(sample.mcc, cell.mcc, sizeof(sample.mcc) - 1);
    strncpy(sample.mnc, cell.mnc, sizeof(sample.mnc) - 1);
    strncpy(sample.utcTimestamp, cell.timestamp, sizeof(sample.utcTimestamp) - 1);

    // Neighbour scan
    NeighbourCell neighbours[8];
    sample.neighbourCount = _analyzer.scanNeighbours(neighbours, 8);
    if (sample.neighbourCount > 0) {
        int maxRSSI = neighbours[0].rssi;
        for (int i = 1; i < sample.neighbourCount; i++)
            if (neighbours[i].rssi > maxRSSI) maxRSSI = neighbours[i].rssi;
        sample.maxNeighRSSI = maxRSSI;
    }

    // GPS data
    GNSSFix fix;
    sample.gpsValid = _gnss.waitForFix(fix, gpsTimeoutMs);
    if (sample.gpsValid) {
        sample.latitude     = fix.latitude;
        sample.longitude    = fix.longitude;
        sample.altitude     = fix.altitude;
        sample.speed        = fix.speed;
        sample.hdop         = fix.hdop;
        sample.gpsSatellites = fix.satellites;
        strncpy(sample.utcTimestamp, fix.utcTime, sizeof(sample.utcTimestamp) - 1);
    }

    // Parse time fields from timestamp "YYYY-MM-DD HH:MM:SS"
    if (strlen(sample.utcTimestamp) >= 16) {
        sample.hourOfDay = atoi(sample.utcTimestamp + 11);
        // dayOfWeek: simple approximation via Zeller's formula omitted here;
        // leave as 0 and populate from RTC if available
    }

    // Label
    strncpy(sample.label, label, sizeof(sample.label) - 1);
    sample.labelConfidence = confidence;

    return true;
}

static const char* CSV_HEADER =
    "timestamp,label,confidence,rssi,rsrp,rsrq,sinr,csq,"
    "cellId,lac,mcc,mnc,neighbours,maxNeighRSSI,"
    "lat,lon,alt,speed,hdop,gpsSats,gpsValid,"
    "hourOfDay,dayOfWeek\n";

void MLDataGen::_fillCSVHeader(char* buf, size_t len) {
    strncpy(buf, CSV_HEADER, len - 1);
}

void MLDataGen::_sampleToCSVRow(const MLSample& s, char* buf, size_t len) {
    snprintf(buf, len,
             "%s,%s,%.2f,"
             "%d,%d,%d,%d,%d,"
             "%d,%d,%s,%s,%d,%d,"
             "%.6f,%.6f,%.2f,%.2f,%.2f,%d,%d,"
             "%d,%d\n",
             s.utcTimestamp, s.label, s.labelConfidence,
             s.rssi, s.rsrp, s.rsrq, s.sinr, s.csq,
             s.cellId, s.lac, s.mcc, s.mnc,
             s.neighbourCount, s.maxNeighRSSI,
             s.latitude, s.longitude, s.altitude, s.speed, s.hdop,
             s.gpsSatellites, s.gpsValid ? 1 : 0,
             s.hourOfDay, s.dayOfWeek);
}

void MLDataGen::_sampleToJSON(const MLSample& s, JsonDocument& doc) {
    doc["ts"]          = s.utcTimestamp;
    doc["label"]       = s.label;
    doc["conf"]        = s.labelConfidence;
    doc["rssi"]        = s.rssi;
    doc["rsrp"]        = s.rsrp;
    doc["rsrq"]        = s.rsrq;
    doc["sinr"]        = s.sinr;
    doc["csq"]         = s.csq;
    doc["cell"]        = s.cellId;
    doc["lac"]         = s.lac;
    doc["mcc"]         = s.mcc;
    doc["mnc"]         = s.mnc;
    doc["nbrs"]        = s.neighbourCount;
    doc["maxNRSSI"]    = s.maxNeighRSSI;
    doc["lat"]         = s.latitude;
    doc["lon"]         = s.longitude;
    doc["alt"]         = s.altitude;
    doc["spd"]         = s.speed;
    doc["hdop"]        = s.hdop;
    doc["gpsSats"]     = s.gpsSatellites;
    doc["gpsOk"]       = s.gpsValid;
    doc["hour"]        = s.hourOfDay;
    doc["dow"]         = s.dayOfWeek;
}

bool MLDataGen::appendCSV(const MLSample& sample, const char* filename) {
    char row[512];
    _sampleToCSVRow(sample, row, sizeof(row));
    _logger.logJSON(LogLevel::INFO, "ML_CSV", row);
    return true;
}

bool MLDataGen::appendJSONL(const MLSample& sample, const char* filename) {
    JsonDocument doc;
    _sampleToJSON(sample, doc);
    char json[512];
    serializeJson(doc, json, sizeof(json));
    _logger.logJSON(LogLevel::INFO, "ML_JSONL", json);
    return true;
}

bool MLDataGen::bufferSample(const MLSample& sample) {
    if (_bufCount >= BATCH_SIZE) return false;
    _buf[_bufCount++] = sample;
    return true;
}

bool MLDataGen::flushBatch(const char* filename) {
    for (int i = 0; i < _bufCount; i++) {
        appendCSV(_buf[i], filename);
    }
    clearBuffer();
    return true;
}

void MLDataGen::clearBuffer() {
    _bufCount = 0;
}
