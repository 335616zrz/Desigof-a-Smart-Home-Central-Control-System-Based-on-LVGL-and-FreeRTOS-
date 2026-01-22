package iot.service.impl;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.integration.mqtt.support.MqttHeaders;
import org.springframework.messaging.MessageChannel;
import org.springframework.messaging.support.MessageBuilder;
import org.springframework.stereotype.Service;

@Service
public class MqttCommandPublisher {

    private static final Logger log = LoggerFactory.getLogger(MqttCommandPublisher.class);

    private final MessageChannel outboundCommandChannel;

    public MqttCommandPublisher(@Qualifier("outboundCommandChannel") MessageChannel outboundCommandChannel) {
        this.outboundCommandChannel = outboundCommandChannel;
    }

    public void publish(String topic, String payload, boolean retained) {
        if (topic == null || topic.isBlank()) {
            throw new IllegalArgumentException("topic is blank");
        }
        if (payload == null) {
            payload = "";
        }

        boolean ok = outboundCommandChannel.send(
                MessageBuilder.withPayload(payload)
                        .setHeader(MqttHeaders.TOPIC, topic)
                        .setHeader(MqttHeaders.QOS, 1)
                        .setHeader(MqttHeaders.RETAINED, retained)
                        .build()
        );

        if (!ok) {
            throw new IllegalStateException("outbound MQTT channel send returned false");
        }

        log.info("MQTT cmd published: topic={} retained={} payload={}", topic, retained, payload);
    }
}
