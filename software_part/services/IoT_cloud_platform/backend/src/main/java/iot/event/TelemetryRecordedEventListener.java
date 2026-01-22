package iot.event;

import iot.service.AlertEvaluatorService;
import iot.service.DeviceShadowService;
import org.springframework.stereotype.Component;
import org.springframework.transaction.event.TransactionPhase;
import org.springframework.transaction.event.TransactionalEventListener;

@Component
public class TelemetryRecordedEventListener {

    private final AlertEvaluatorService alertEvaluatorService;
    private final DeviceShadowService deviceShadowService;

    public TelemetryRecordedEventListener(AlertEvaluatorService alertEvaluatorService,
                                         DeviceShadowService deviceShadowService) {
        this.alertEvaluatorService = alertEvaluatorService;
        this.deviceShadowService = deviceShadowService;
    }

    @TransactionalEventListener(phase = TransactionPhase.AFTER_COMMIT)
    public void onTelemetryRecorded(TelemetryRecordedEvent event) {
        // Update device shadow ("reported") if payload contains state fields.
        deviceShadowService.applyReportedFromTelemetry(
                event.getRecord().getDeviceId(),
                event.getRecord().getPayload(),
                event.getRecord().getReportedAt());

        alertEvaluatorService.evaluateRules(event.getRecord());
    }
}
