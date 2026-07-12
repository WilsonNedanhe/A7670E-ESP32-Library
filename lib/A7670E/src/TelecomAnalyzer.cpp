#include "TelecomAnalyzer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

TelecomAnalyzer::TelecomAnalyzer(ATClient& at,
                                                Network& net,
                                                Logger* logger)
    : _at(at), _net(net), _logger(logger) {}

bool TelecomAnalyzer::sampleServingCell(CellRecord& rec) {
    SignalInfo sig;
    if (!_net.getSignalInfo(sig)) return false;

    memset(&rec, 0, sizeof(rec));
    rec.rssi   = sig.rssi;
    rec.rsrp   = sig.rsrp;
    rec.rsrq   = sig.rsrq;
    rec.sinr   = sig.sinr;
    rec.csq    = sig.csq;
    rec.cellId = sig.cellId;
    rec.lac    = sig.lac;
    rec.tech   = sig.tech;
    strncpy(rec.operatorName, sig.operatorName, sizeof(rec.operatorName) - 1);
    strncpy(rec.mcc, sig.mcc, sizeof(rec.mcc) - 1);
    strncpy(rec.mnc, sig.mnc, sizeof(rec.mnc) - 1);

    // Store in ring buffer
    _history[_histHead] = rec;
    _histHead = (_histHead + 1) % HISTORY_SIZE;
    if (_histCount < HISTORY_SIZE) _histCount++;

    if (_logger) {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "{\"rssi\":%d,\"rsrp\":%d,\"cell\":%d,\"op\":\"%s\"}",
                 rec.rssi, rec.rsrp, rec.cellId, rec.operatorName);
        _logger->logJSON(LogLevel::INFO, "CELL", msg);
    }
    return true;
}

int TelecomAnalyzer::scanNeighbours(NeighbourCell* cells, int maxCells) {
    char resp[1024];
    if (_at.sendCmd("AT+CEREG=5", "OK") != ATResult::OK) return 0;
    if (_at.sendCmd("AT+CEREG?", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return 0;

    int count = 0;
    char* p = resp;
    // Parse neighbour cells from extended CEREG response
    while ((p = strstr(p, "\"earfcn\"")) && count < maxCells) {
        int earfcn, pci, rsrp, rsrq;
        if (sscanf(p, "\"earfcn\":%d,\"pci\":%d,\"rsrp\":%d,\"rsrq\":%d",
                   &earfcn, &pci, &rsrp, &rsrq) == 4) {
            cells[count].cellId = pci;
            cells[count].rsrp   = rsrp;
            cells[count].rssi   = rsrp;
            cells[count].tech   = NetTech::LTE;
            count++;
        }
        p++;
    }
    return count;
}

bool TelecomAnalyzer::runDiagnostics(DiagReport& report, const char* pingHost) {
    memset(&report, 0, sizeof(report));

    if (!sampleServingCell(report.serving)) return false;

    report.neighbourCount = scanNeighbours(report.neighbours, 8);
    report.dataConnected  = _net.isConnected();

    if (report.dataConnected) {
        uint32_t t0 = millis();
        _net.ping(pingHost, 10000);
        report.pingLatencyMs = millis() - t0;
    }

    if (_logger) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "{\"connected\":%s,\"ping_ms\":%u,\"neighbours\":%d}",
                 report.dataConnected ? "true" : "false",
                 (unsigned)report.pingLatencyMs,
                 report.neighbourCount);
        _logger->logJSON(LogLevel::INFO, "DIAG", msg);
    }
    return true;
}

void TelecomAnalyzer::startContinuousSampling(uint32_t intervalMs) {
    _sampling = true;
    // Sampling is driven by the caller's loop() — see example sketch.
    // A FreeRTOS task variant can be added per target application.
    (void)intervalMs;
}

void TelecomAnalyzer::stopContinuousSampling() {
    _sampling = false;
}

const CellRecord* TelecomAnalyzer::history(int& count) const {
    count = _histCount;
    return _history;
}
