package iot.service;

import iot.domain.TelemetryRecord;

/**
 * Service for evaluating alert rules against telemetry data
 */
public interface AlertEvaluatorService {

    /**
     * Evaluate all enabled alert rules for a telemetry record
     * @param record The telemetry record to evaluate
     */
    void evaluateRules(TelemetryRecord record);
}
