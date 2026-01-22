package iot.service.impl;

import iot.domain.AlertRule;
import iot.mapper.AlertRuleMapper;
import iot.service.AlertRuleService;
import iot.service.SeasonService;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@Transactional(readOnly = true)
public class AlertRuleServiceImpl implements AlertRuleService {

    private final AlertRuleMapper alertRuleMapper;
    private final SeasonService seasonService;

    public AlertRuleServiceImpl(AlertRuleMapper alertRuleMapper, SeasonService seasonService) {
        this.alertRuleMapper = alertRuleMapper;
        this.seasonService = seasonService;
    }

    @Override
    public List<AlertRule> listRules(Boolean enabled) {
        return alertRuleMapper.findAll(enabled);
    }

    @Override
    public Optional<AlertRule> findById(Long id) {
        return Optional.ofNullable(alertRuleMapper.findById(id));
    }

    @Override
    public List<AlertRule> findByDeviceId(String deviceId) {
        return alertRuleMapper.findByDeviceId(deviceId);
    }

    @Override
    public List<AlertRule> findEnabledRules() {
        return alertRuleMapper.findEnabledRules();
    }

    @Override
    @Transactional
    public AlertRule createRule(AlertRule rule) {
        if (Boolean.TRUE.equals(rule.getIsSystemTemplate())) {
            throw new IllegalArgumentException("Cannot manually create system template");
        }

        rule.setIsSystemTemplate(false);
        if (rule.getDeviceId() != null && rule.getDeviceId().trim().isEmpty()) {
            rule.setDeviceId(null);
        }
        LocalDateTime now = LocalDateTime.now();
        if (rule.getCreatedAt() == null) {
            rule.setCreatedAt(now);
        }
        if (rule.getUpdatedAt() == null) {
            rule.setUpdatedAt(now);
        }
        if (rule.getEnabled() == null) {
            rule.setEnabled(true);
        }
        if (rule.getDurationSeconds() == null) {
            rule.setDurationSeconds(60);
        }
        alertRuleMapper.insert(rule);
        return rule;
    }

    @Override
    @Transactional
    public AlertRule updateRule(AlertRule rule) {
        AlertRule existing = alertRuleMapper.findById(rule.getId());
        if (existing == null) {
            throw new IllegalArgumentException("Alert rule not found: " + rule.getId());
        }

        if (Boolean.TRUE.equals(existing.getIsSystemTemplate())) {
            // System template is read-only except enabled toggle
            alertRuleMapper.updateEnabled(rule.getId(), rule.getEnabled());
            existing.setEnabled(rule.getEnabled());
            return existing;
        }

        rule.setIsSystemTemplate(false);
        if (rule.getDeviceId() != null && rule.getDeviceId().trim().isEmpty()) {
            rule.setDeviceId(null);
        }
        rule.setUpdatedAt(LocalDateTime.now());
        alertRuleMapper.update(rule);
        return rule;
    }

    @Override
    @Transactional
    public void deleteRule(Long id) {
        int deleted = alertRuleMapper.deleteByIdIfNotSystem(id);
        if (deleted == 0) {
            throw new IllegalStateException("Cannot delete system template or rule not found");
        }
    }

    @Override
    @Transactional
    public void updateEnabled(Long id, Boolean enabled) {
        alertRuleMapper.updateEnabled(id, enabled);
    }

    @Override
    public List<AlertRule> listActiveRulesForCurrentSeason(Boolean enabled) {
        String season = seasonService.getCurrentSeason();
        return alertRuleMapper.findActiveRulesBySeason(season, enabled);
    }

    @Override
    public List<AlertRule> listSystemTemplates(String season) {
        return alertRuleMapper.findSystemTemplates(season);
    }

    @Override
    public List<AlertRule> listUserRules(String deviceId) {
        return alertRuleMapper.findUserRules(deviceId);
    }
}
