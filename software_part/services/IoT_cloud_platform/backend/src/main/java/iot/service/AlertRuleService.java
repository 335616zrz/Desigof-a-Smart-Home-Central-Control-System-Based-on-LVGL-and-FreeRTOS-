package iot.service;

import iot.domain.AlertRule;
import java.util.List;
import java.util.Optional;

public interface AlertRuleService {

    List<AlertRule> listRules(Boolean enabled);

    Optional<AlertRule> findById(Long id);

    List<AlertRule> findByDeviceId(String deviceId);

    List<AlertRule> findEnabledRules();

    AlertRule createRule(AlertRule rule);

    AlertRule updateRule(AlertRule rule);

    void deleteRule(Long id);

    void updateEnabled(Long id, Boolean enabled);

    List<AlertRule> listActiveRulesForCurrentSeason(Boolean enabled);

    List<AlertRule> listSystemTemplates(String season);

    List<AlertRule> listUserRules(String deviceId);
}
