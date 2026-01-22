package iot.controller;

import iot.domain.Device;
import iot.service.DeviceService;
import java.util.List;
import org.springframework.messaging.handler.annotation.MessageMapping;
import org.springframework.messaging.handler.annotation.SendTo;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Controller;

@Controller
public class TelemetryWebSocketController {

    private final DeviceService deviceService;
    private final SimpMessagingTemplate messagingTemplate;

    public TelemetryWebSocketController(DeviceService deviceService, SimpMessagingTemplate messagingTemplate) {
        this.deviceService = deviceService;
        this.messagingTemplate = messagingTemplate;
    }

    @MessageMapping("/devices/refresh")
    @SendTo("/topic/devices")
    public List<Device> refresh() {
        return deviceService.listDevices();
    }

    @Scheduled(fixedDelayString = "${iot.websocket.refresh-interval-ms:10000}")
    public void heartbeat() {
        messagingTemplate.convertAndSend("/topic/devices", deviceService.listDevices());
    }
}
