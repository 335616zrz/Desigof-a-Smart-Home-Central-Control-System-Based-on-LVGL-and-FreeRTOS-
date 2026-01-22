package iot.service.impl;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import iot.domain.Alert;
import iot.domain.AlertRule;
import iot.domain.TelemetryRecord;
import iot.mapper.TelemetryMapper;
import iot.service.AlertEvaluatorService;
import iot.service.AlertRuleService;
import iot.service.AlertService;
import java.time.LocalDateTime;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Service;

@Service
public class AlertEvaluatorServiceImpl implements AlertEvaluatorService {

    private static final Logger log = LoggerFactory.getLogger(AlertEvaluatorServiceImpl.class);

    private final AlertRuleService alertRuleService;
    private final AlertService alertService;
    private final TelemetryMapper telemetryMapper;
    private final ObjectMapper objectMapper;

    public AlertEvaluatorServiceImpl(
            AlertRuleService alertRuleService,
            AlertService alertService,
            TelemetryMapper telemetryMapper,
            ObjectMapper objectMapper) {
        this.alertRuleService = alertRuleService;
        this.alertService = alertService;
        this.telemetryMapper = telemetryMapper;
        this.objectMapper = objectMapper;
    }

    @Override
    @Async("telemetryExecutor")
    public void evaluateRules(TelemetryRecord record) {
        try {
            String deviceId = record.getDeviceId();

            // Get all enabled rules for this device (device-specific + global rules)
            List<AlertRule> rules = alertRuleService.listActiveRulesForCurrentSeason(true);

            for (AlertRule rule : rules) {
                // Skip if rule is device-specific and doesn't match
                if (rule.getDeviceId() != null && !rule.getDeviceId().equals(deviceId)) {
                    continue;
                }

                evaluateRule(rule, record);
            }
        } catch (Exception e) {
            log.error("Error evaluating alert rules for device {}", record.getDeviceId(), e);
        }
    }

    private void evaluateRule(AlertRule rule, TelemetryRecord record) {
        try {
            // Extract metric value from payload
            Double metricValue = extractMetric(record.getPayload(), rule.getMetric());
            if (metricValue == null) {
                log.debug("Metric {} not found in payload for device {}", rule.getMetric(), record.getDeviceId());
                return;
            }

            // Check if threshold is violated
            boolean violated = checkThreshold(metricValue, rule.getOperator(), rule.getThreshold());

            if (!violated) {
                log.debug("Rule {} not violated: {} {} {}", rule.getId(), metricValue, rule.getOperator(), rule.getThreshold());
                return;
            }

            // Check duration (if the violation persists for the required duration)
            if (rule.getDurationSeconds() != null && rule.getDurationSeconds() > 0) {
                LocalDateTime endTime = record.getReportedAt() != null ? record.getReportedAt() : LocalDateTime.now();
                LocalDateTime since = endTime.minusSeconds(rule.getDurationSeconds());
                if (!checkPersistentViolation(rule, record.getDeviceId(), since, endTime)) {
                    log.debug("Rule {} violated but not persistent enough", rule.getId());
                    return;
                }
            }

            // Check if there's already an open alert for this rule and device
            Alert existingAlert = alertService.findOpenByRuleAndDevice(rule.getId(), record.getDeviceId());
            if (existingAlert != null) {
                log.debug("Alert already exists for rule {} and device {}", rule.getId(), record.getDeviceId());
                return;
            }

            // Create new alert
            Alert alert = new Alert();
            alert.setDeviceId(record.getDeviceId());
            alert.setRuleId(rule.getId());
            alert.setLevel(rule.getLevel());
            alert.setMessage(String.format(
                "%s: %s %s %.2f (threshold: %.2f)",
                rule.getName(),
                rule.getMetric(),
                rule.getOperator(),
                metricValue,
                rule.getThreshold()
            ));
            alert.setStatus("open");
            alert.setCreatedAt(LocalDateTime.now());

            alertService.createAlert(alert);
            log.info("Alert created: rule={}, device={}, metric={}, value={}",
                rule.getId(), record.getDeviceId(), rule.getMetric(), metricValue);

        } catch (Exception e) {
            log.error("Error evaluating rule {} for device {}", rule.getId(), record.getDeviceId(), e);
        }
    }

    private Double extractMetric(String payload, String metric) {
        try {
            JsonNode root = objectMapper.readTree(payload);

            // Try to get metric from data object first
            JsonNode dataNode = root.has("data") ? root.get("data") : root;

            if (dataNode.has(metric)) {
                JsonNode metricNode = dataNode.get(metric);
                if (metricNode.isNumber()) {
                    return metricNode.asDouble();
                }
            }

            return null;
        } catch (Exception e) {
            log.warn("Failed to extract metric {} from payload", metric, e);
            return null;
        }
    }

    private boolean checkThreshold(double value, String operator, double threshold) {
        switch (operator) {
            case ">":
                return value > threshold;
            case ">=":
                return value >= threshold;
            case "<":
                return value < threshold;
            case "<=":
                return value <= threshold;
            case "==":
            case "=":
                return Math.abs(value - threshold) < 0.0001; // Handle floating point comparison
            case "!=":
                return Math.abs(value - threshold) >= 0.0001;
            default:
                log.warn("Unknown operator: {}", operator);
                return false;
        }
    }

    private boolean checkPersistentViolation(AlertRule rule, String deviceId, LocalDateTime since, LocalDateTime endTime) {
        // Get recent telemetry records within the duration window using mapper directly
        List<TelemetryRecord> recentRecords = telemetryMapper.findByDeviceAndRange(
            deviceId,
            since,
            endTime,
            100
        );

        if (recentRecords.isEmpty()) {
            log.debug("No telemetry records found for device {} since {}", deviceId, since);
            return false;
        }

        // Check if violation persists throughout the duration window
        int violationCount = 0;
        int totalCount = 0;

        for (TelemetryRecord record : recentRecords) {
            Double metricValue = extractMetric(record.getPayload(), rule.getMetric());
            if (metricValue != null) {
                totalCount++;
                if (checkThreshold(metricValue, rule.getOperator(), rule.getThreshold())) {
                    violationCount++;
                }
            }
        }

        log.debug("Persistent violation check for rule {}: {}/{} records violate threshold",
                  rule.getId(), violationCount, totalCount);

        // If we have at least 1 record and all records violate the threshold, trigger alert
        // This ensures the violation truly persists throughout the duration window
        if (totalCount == 0) {
            return false;
        }

        // If we have only 1-2 records, require 100% violation rate
        // If we have more records, require at least 80% violation rate
        double requiredRate = totalCount <= 2 ? 1.0 : 0.8;
        return (violationCount * 1.0 / totalCount) >= requiredRate;
    }
}
