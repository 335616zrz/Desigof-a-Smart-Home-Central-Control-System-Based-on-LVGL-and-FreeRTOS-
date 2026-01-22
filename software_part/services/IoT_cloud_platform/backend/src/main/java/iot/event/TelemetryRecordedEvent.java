package iot.event;

import iot.domain.TelemetryRecord;

public class TelemetryRecordedEvent {

    private final TelemetryRecord record;

    public TelemetryRecordedEvent(TelemetryRecord record) {
        this.record = record;
    }

    public TelemetryRecord getRecord() {
        return record;
    }
}

