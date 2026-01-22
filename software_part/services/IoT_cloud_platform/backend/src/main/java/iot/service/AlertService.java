package iot.service;

import iot.domain.Alert;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

public interface AlertService {

    List<Alert> listAlerts(String status, String level, Integer page, Integer pageSize);

    Long countAlerts(String status, String level);

    Optional<Alert> findById(Long id);

    List<Alert> findByDeviceId(String deviceId);

    Alert findOpenByRuleAndDevice(Long ruleId, String deviceId);

    Alert createAlert(Alert alert);

    void acknowledge(Long id);

    void close(Long id);

    void deleteAlert(Long id);

    List<Alert> listAlertsForExport(String status, String level, String deviceId, LocalDateTime from, LocalDateTime to);
}
