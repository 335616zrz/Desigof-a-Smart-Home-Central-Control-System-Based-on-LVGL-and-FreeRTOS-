package iot.service.impl;

import iot.service.DeviceService;
import iot.service.TelemetryIngestService;
import iot.service.TelemetryService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Component;

@Component
public class TelemetryIngestServiceImpl implements TelemetryIngestService {

    private static final Logger log = LoggerFactory.getLogger(TelemetryIngestServiceImpl.class);

    private final DeviceService deviceService;
    private final TelemetryService telemetryService;

    public TelemetryIngestServiceImpl(DeviceService deviceService, TelemetryService telemetryService) {
        this.deviceService = deviceService;
        this.telemetryService = telemetryService;
    }

    @Override
    @Async("telemetryExecutor")
    public void handleMessage(String topic, String payload) {
        log.debug("Telemetry message from topic {}: {}", topic, payload);
        String deviceId = topic.replace("devices/", "").replace("/state", "");
        deviceService.updateStatus(deviceId, "ONLINE");
        if (payload != null && !payload.isEmpty()) {
            telemetryService.recordTelemetry(deviceId, payload);
        }
    }

}
