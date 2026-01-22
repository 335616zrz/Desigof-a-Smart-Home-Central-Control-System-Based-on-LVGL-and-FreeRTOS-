package iot.service.impl;

import iot.domain.Alert;
import iot.mapper.AlertMapper;
import iot.service.AlertService;
import iot.service.DeviceShadowService;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@Transactional(readOnly = true)
public class AlertServiceImpl implements AlertService {

    private static final Logger log = LoggerFactory.getLogger(AlertServiceImpl.class);

    private final AlertMapper alertMapper;
    private final MqttCommandPublisher mqttCommandPublisher;
    private final DeviceShadowService deviceShadowService;

    public AlertServiceImpl(AlertMapper alertMapper,
                            MqttCommandPublisher mqttCommandPublisher,
                            DeviceShadowService deviceShadowService) {
        this.alertMapper = alertMapper;
        this.mqttCommandPublisher = mqttCommandPublisher;
        this.deviceShadowService = deviceShadowService;
    }

    @Override
    public List<Alert> listAlerts(String status, String level, Integer page, Integer pageSize) {
        Integer offset = null;
        Integer limit = null;
        if (page != null && pageSize != null) {
            offset = page * pageSize;
            limit = pageSize;
        }
        return alertMapper.findAll(status, level, limit, offset);
    }

    @Override
    public Long countAlerts(String status, String level) {
        return alertMapper.countAll(status, level);
    }

    @Override
    public Optional<Alert> findById(Long id) {
        return Optional.ofNullable(alertMapper.findById(id));
    }

    @Override
    public List<Alert> findByDeviceId(String deviceId) {
        return alertMapper.findByDeviceId(deviceId);
    }

    @Override
    public Alert findOpenByRuleAndDevice(Long ruleId, String deviceId) {
        return alertMapper.findOpenByRuleAndDevice(ruleId, deviceId);
    }

    @Override
    @Transactional
    public Alert createAlert(Alert alert) {
        if (alert.getCreatedAt() == null) {
            alert.setCreatedAt(LocalDateTime.now());
        }
        if (alert.getStatus() == null) {
            alert.setStatus("open");
        }
        alertMapper.insert(alert);
        syncAlarmIndicator(alert.getDeviceId());
        return alert;
    }

    @Override
    @Transactional
    public void acknowledge(Long id) {
        Alert existing = alertMapper.findById(id);
        if (existing == null) return;
        alertMapper.acknowledge(id, LocalDateTime.now());
        syncAlarmIndicator(existing.getDeviceId());
    }

    @Override
    @Transactional
    public void close(Long id) {
        Alert existing = alertMapper.findById(id);
        if (existing == null) return;
        alertMapper.close(id, LocalDateTime.now());
        syncAlarmIndicator(existing.getDeviceId());
    }

    @Override
    @Transactional
    public void deleteAlert(Long id) {
        Alert existing = alertMapper.findById(id);
        alertMapper.deleteById(id);
        if (existing != null) {
            syncAlarmIndicator(existing.getDeviceId());
        }
    }

    @Override
    public List<Alert> listAlertsForExport(String status, String level, String deviceId, LocalDateTime from, LocalDateTime to) {
        return alertMapper.findAllForExport(status, level, deviceId, from, to);
    }

    private void syncAlarmIndicator(String deviceId) {
        if (deviceId == null || deviceId.isBlank()) return;
        Long openCount = alertMapper.countOpenByDevice(deviceId);
        boolean active = openCount != null && openCount > 0;

        String topic = "devices/" + deviceId + "/command/alarm";
        String payload = "{\"cmd\":\"alarm\",\"active\":" + (active ? "true" : "false") + ",\"deviceId\":\"" + deviceId + "\"}";
        try {
            mqttCommandPublisher.publish(topic, payload, true);
            deviceShadowService.setDesiredAlarmActive(deviceId, active);
        } catch (Exception ex) {
            log.warn("Failed to publish MQTT alarm indicator. deviceId={} active={}", deviceId, active, ex);
        }
    }
}
