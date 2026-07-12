#pragma once
#include "ATClient.h"
#include "Network.h"
#include "Logger.h"

// Phase 11 — Telecom Analyzer / Network Diagnostics
// Continuous signal sampling, neighbour cell scan, latency measurement.

struct CellRecord {
    char   timestamp[24];
    char   operatorName[32];
    char   mcc[4];
    char   mnc[4];
    int    lac;
    int    cellId;
    int    rssi;
    int    rsrp;
    int    rsrq;
    int    sinr;
    int    csq;
    NetTech tech;
};

struct NeighbourCell {
    int  cellId;
    int  lac;
    int  rssi;
    int  rsrp;
    NetTech tech;
};

struct DiagReport {
    CellRecord    serving;
    NeighbourCell neighbours[8];
    int           neighbourCount;
    uint32_t      pingLatencyMs;
    bool          dataConnected;
};

class TelecomAnalyzer {
public:
    TelecomAnalyzer(ATClient& at,
                           Network& net,
                           Logger*  logger = nullptr);

    // Sample serving cell once
    bool      sampleServingCell(CellRecord& rec);

    // Scan neighbour cells (CEREG / AT+QENG style)
    int       scanNeighbours(NeighbourCell* cells, int maxCells);

    // Full diagnostic snapshot
    bool      runDiagnostics(DiagReport& report, const char* pingHost = "8.8.8.8");

    // Continuous sampling loop — calls sampleServingCell every intervalMs
    // and logs to SD if logger != nullptr. Call from a FreeRTOS task or loop().
    void      startContinuousSampling(uint32_t intervalMs = 5000);
    void      stopContinuousSampling();
    bool      isSampling() const { return _sampling; }

    // Returns pointer to ring-buffer of last N samples
    const CellRecord* history(int& count) const;

private:
    ATClient&      _at;
    Network& _net;
    Logger*  _logger;
    bool            _sampling = false;

    static constexpr int HISTORY_SIZE = 64;
    CellRecord _history[HISTORY_SIZE];
    int        _histHead  = 0;
    int        _histCount = 0;
};
