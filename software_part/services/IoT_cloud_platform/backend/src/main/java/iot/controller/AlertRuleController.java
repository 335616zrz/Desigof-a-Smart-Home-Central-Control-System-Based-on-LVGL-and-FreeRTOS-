package iot.controller;

import iot.domain.AlertRule;
import iot.dto.SeasonResponse;
import iot.service.AlertRuleService;
import iot.service.SeasonService;
import jakarta.validation.Valid;
import java.util.List;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/alert-rules")
public class AlertRuleController {

    private final AlertRuleService alertRuleService;
    private final SeasonService seasonService;

    public AlertRuleController(AlertRuleService alertRuleService, SeasonService seasonService) {
        this.alertRuleService = alertRuleService;
        this.seasonService = seasonService;
    }

    @GetMapping
    public List<AlertRule> listRules(@RequestParam(required = false) Boolean enabled) {
        return alertRuleService.listRules(enabled);
    }

    @GetMapping("/current-season")
    public SeasonResponse currentSeason() {
        String season = seasonService.getCurrentSeason();
        return new SeasonResponse(season, getSeasonDisplayName(season));
    }

    @GetMapping("/templates")
    public List<AlertRule> listSystemTemplates(@RequestParam(required = false) String season) {
        return alertRuleService.listSystemTemplates(season);
    }

    @GetMapping("/user-rules")
    public List<AlertRule> listUserRules(@RequestParam(required = false) String deviceId) {
        return alertRuleService.listUserRules(deviceId);
    }

    @GetMapping("/{id}")
    public ResponseEntity<AlertRule> getRule(@PathVariable Long id) {
        return alertRuleService.findById(id)
                .map(ResponseEntity::ok)
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    @GetMapping("/device/{deviceId}")
    public List<AlertRule> getRulesByDevice(@PathVariable String deviceId) {
        return alertRuleService.findByDeviceId(deviceId);
    }

    @PostMapping
    @PreAuthorize("hasAnyAuthority('ROLE_ADMIN','ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_OPERATOR')")
    public ResponseEntity<AlertRule> createRule(@Valid @RequestBody AlertRule rule) {
        if (Boolean.TRUE.equals(rule.getIsSystemTemplate())) {
            return ResponseEntity.badRequest().build();
        }
        rule.setIsSystemTemplate(false);
        AlertRule created = alertRuleService.createRule(rule);
        return ResponseEntity.status(HttpStatus.CREATED).body(created);
    }

    @PutMapping("/{id}")
    @PreAuthorize("hasAnyAuthority('ROLE_ADMIN','ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_OPERATOR')")
    public ResponseEntity<AlertRule> updateRule(@PathVariable Long id, @Valid @RequestBody AlertRule rule) {
        Optional<AlertRule> existingOpt = alertRuleService.findById(id);
        if (existingOpt.isEmpty()) {
            return ResponseEntity.notFound().build();
        }

        AlertRule existing = existingOpt.get();

        if (Boolean.TRUE.equals(existing.getIsSystemTemplate())) {
            if (rule.getEnabled() == null) {
                return ResponseEntity.badRequest().build();
            }
            alertRuleService.updateEnabled(id, rule.getEnabled());
            existing.setEnabled(rule.getEnabled());
            return ResponseEntity.ok(existing);
        }

        rule.setId(id);
        rule.setIsSystemTemplate(false);
        AlertRule updated = alertRuleService.updateRule(rule);
        return ResponseEntity.ok(updated);
    }

    @PatchMapping("/{id}/enabled")
    @PreAuthorize("hasAnyAuthority('ROLE_ADMIN','ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_OPERATOR')")
    public ResponseEntity<Void> toggleRule(@PathVariable Long id, @RequestParam Boolean enabled) {
        alertRuleService.updateEnabled(id, enabled);
        return ResponseEntity.ok().build();
    }

    @DeleteMapping("/{id}")
    @PreAuthorize("hasAnyAuthority('ROLE_ADMIN','ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_OPERATOR')")
    public ResponseEntity<Void> deleteRule(@PathVariable Long id) {
        try {
            alertRuleService.deleteRule(id);
            return ResponseEntity.noContent().build();
        } catch (IllegalStateException e) {
            return ResponseEntity.badRequest().build();
        }
    }

    private String getSeasonDisplayName(String season) {
        return switch (season) {
            case "summer" -> "夏季";
            case "winter" -> "冬季";
            case "transition" -> "过渡季";
            default -> "未知";
        };
    }
}
