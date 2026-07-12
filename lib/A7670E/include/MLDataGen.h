#pragma once
#include "TelecomAnalyzer.h"
#include "GPS.h"
#include "Logger.h"
#include <ArduinoJson.h>

// Phase 12 — Machine Learning Dataset Generator
// Collects labelled (cell + GPS + time) feature vectors and writes CSV / JSON Lines.

struct MLSample {
    // ── Cell features ──
    int    rssi;
    int    rsrp;
    int    rsrq;
    int    sinr;
    int    csq;
    int    cellId;
    int    lac;
    char   mcc[4];
    char   mnc[4];
    int    neighbourCount;
    int    maxNeighRSSI;

    // ── Location features ──
    float  latitude;
    float  longitude;
    float  altitude;
    float  speed;
    float  hdop;
    int    gpsSatellites;
    bool   gpsValid;

    // ── Time features ──
    char   utcTimestamp[24];
    int    hourOfDay;      // 0-23
    int    dayOfWeek;      // 0-6

    // ── Label (user-assigned) ──
    char   label[32];      // e.g. "indoor", "outdoor", "moving", "stationary"
    float  labelConfidence; // 0.0-1.0
};

class MLDataGen {
public:
    MLDataGen(TelecomAnalyzer& analyzer,
                     GPS&            gnss,
                     Logger&          logger);

    // Collect one sample (blocks until GPS fix or gpsTimeoutMs elapses)
    bool collectSample(MLSample& sample,
                       const char* label = "unlabeled",
                       float confidence  = 1.0f,
                       uint32_t gpsTimeoutMs = 10000);

    // Append sample to CSV file on SD (creates header if new file)
    bool appendCSV (const MLSample& sample, const char* filename = "/ml_dataset.csv");

    // Append sample as JSON Lines entry on SD
    bool appendJSONL(const MLSample& sample, const char* filename = "/ml_dataset.jsonl");

    // Write in-memory buffer of N samples to file in one shot
    bool flushBatch(const char* filename = "/ml_dataset.csv");

    // Buffer a sample for batch write (returns false when buffer full)
    bool bufferSample(const MLSample& sample);

    void clearBuffer();
    int  bufferedCount() const { return _bufCount; }

private:
    TelecomAnalyzer& _analyzer;
    GPS&            _gnss;
    Logger&          _logger;

    static constexpr int BATCH_SIZE = 32;
    MLSample _buf[BATCH_SIZE];
    int      _bufCount = 0;

    void _fillCSVHeader(char* buf, size_t len);
    void _sampleToCSVRow(const MLSample& s, char* buf, size_t len);
    void _sampleToJSON  (const MLSample& s, JsonDocument& doc);
};
