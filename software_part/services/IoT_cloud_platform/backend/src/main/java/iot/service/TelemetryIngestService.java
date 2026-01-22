package iot.service;

public interface TelemetryIngestService {

    void handleMessage(String topic, String payload);
}
