package iot.service.impl;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import iot.domain.Alert;
import iot.domain.AlertRule;
import iot.domain.TelemetryRecord;
import iot.service.AlertRuleService;
import iot.service.AlertService;
import iot.service.TelemetryService;
import java.time.LocalDateTime;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

/**
 * Very lightweight periodic evaluator to turn enabled alert rules into alerts based on latest telemetry.
 */
@Component
public class AlertEvaluationJob {

    private static final Logger log = LoggerFactory.getLogger(AlertEvaluationJob.class);

    private final AlertRuleService alertRuleService;
    private final TelemetryService telemetryService;
    private final AlertService alertService;
    private final ObjectMapper objectMapper;

    public AlertEvaluationJob(AlertRuleService alertRuleService,
                              TelemetryService telemetryService,
                              AlertService alertService,
                              ObjectMapper objectMapper) {
        this.alertRuleService = alertRuleService;
        this.telemetryService = telemetryService;
        this.alertService = alertService;
        this.objectMapper = objectMapper;
    }

    @Scheduled(fixedDelayString = "${iot.alerts.evaluation-interval-ms:15000}")
    public void evaluate() {
        List<AlertRule> rules = alertRuleService.findEnabledRules();
        LocalDateTime now = LocalDateTime.now();
        for (AlertRule rule : rules) {
            String deviceId = rule.getDeviceId();
            if (deviceId == null) {
                continue; // only per-device rules for now
            }
            LocalDateTime since = now.minusSeconds(rule.getDurationSeconds() == null ? 60 : rule.getDurationSeconds());
            TelemetryRecord latest = telemetryService.findLatestWithin(deviceId, since);
            if (latest == null) continue;
            Double metricValue = extractMetric(latest.getPayload(), rule.getMetric());
            if (metricValue == null) continue;
            if (matches(rule.getOperator(), metricValue, rule.getThreshold())) {
                // Avoid duplicate open alerts
                Alert existing = alertService.findOpenByRuleAndDevice(rule.getId(), deviceId);
                if (existing != null) continue;
                Alert alert = new Alert();
                alert.setDeviceId(deviceId);
                alert.setRuleId(rule.getId());
                alert.setLevel(rule.getLevel());
                alert.setMessage(String.format("规则[%s]触发：%s %s %.2f (当前 %.2f)",
                        rule.getName(), rule.getMetric(), rule.getOperator(), rule.getThreshold(), metricValue));
                alert.setStatus("open");
                alert.setCreatedAt(now);
                alertService.createAlert(alert);
            }
        }
    }

    private Double extractMetric(String payload, String metric) {
        try {
            JsonNode node = objectMapper.readTree(payload);
            JsonNode data = node.has("data") ? node.get("data") : node;
            JsonNode valueNode = data.get(metric);
            if (valueNode != null && valueNode.isNumber()) {
                return valueNode.asDouble();
            }
        } catch (Exception e) {
            log.debug("Failed to parse telemetry payload for alert evaluation", e);
        }
        return null;
    }

    private boolean matches(String operator, double value, double threshold) {
        return switch (operator) {
            case ">" -> value > threshold;
            case ">=" -> value >= threshold;
            case "<" -> value < threshold;
            case "<=" -> value <= threshold;
            case "==" -> value == threshold;
            case "!=" -> value != threshold;
            default -> false;
        };
    }
}
