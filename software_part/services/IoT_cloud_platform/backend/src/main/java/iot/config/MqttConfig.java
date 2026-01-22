package iot.config;

import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.integration.channel.DirectChannel;
import org.springframework.integration.core.MessageProducer;
import org.springframework.integration.mqtt.core.DefaultMqttPahoClientFactory;
import org.springframework.integration.mqtt.core.MqttPahoClientFactory;
import org.springframework.integration.mqtt.inbound.MqttPahoMessageDrivenChannelAdapter;
import org.springframework.integration.mqtt.outbound.MqttPahoMessageHandler;
import org.springframework.messaging.MessageChannel;
import org.springframework.messaging.MessageHandler;

@Configuration
public class MqttConfig {

    @Value("${iot.mqtt.broker:tcp://localhost:1883}")
    private String brokerUrl;

    @Value("${iot.mqtt.username:admin}")
    private String username;

    @Value("${iot.mqtt.password:public}")
    private String password;

    @Value("${iot.mqtt.client-id:cloud-core}")
    private String clientId;

    @Value("${iot.mqtt.topic.devices:devices/+/state}")
    private String deviceTopic;

    @Bean
    public MqttPahoClientFactory mqttClientFactory() {
        DefaultMqttPahoClientFactory factory = new DefaultMqttPahoClientFactory();
        MqttConnectOptions options = new MqttConnectOptions();
        options.setServerURIs(new String[]{brokerUrl});
        options.setAutomaticReconnect(true);
        options.setCleanSession(false);
        options.setUserName(username);
        options.setPassword(password.toCharArray());
        factory.setConnectionOptions(options);
        return factory;
    }

    @Bean
    public MessageChannel inboundTelemetryChannel() {
        return new DirectChannel();
    }

    @Bean
    public MessageProducer inboundTelemetryAdapter(MqttPahoClientFactory mqttClientFactory) {
        MqttPahoMessageDrivenChannelAdapter adapter =
                new MqttPahoMessageDrivenChannelAdapter(clientId + "-in", mqttClientFactory, deviceTopic);
        adapter.setOutputChannel(inboundTelemetryChannel());
        adapter.setManualAcks(false);
        adapter.setQos(1);
        return adapter;
    }

    @Bean
    @ServiceActivator(inputChannel = "inboundTelemetryChannel")
    public MessageHandler inboundTelemetryHandler(iot.service.TelemetryIngestService telemetryIngestService) {
        return message -> {
            String topic = (String) message.getHeaders().get("mqtt_receivedTopic");
            Object payload = message.getPayload();
            telemetryIngestService.handleMessage(topic, payload == null ? "" : payload.toString());
        };
    }

    @Bean
    public MessageChannel outboundCommandChannel() {
        return new DirectChannel();
    }

    @Bean
    @ServiceActivator(inputChannel = "outboundCommandChannel")
    public MessageHandler mqttOutbound(MqttPahoClientFactory mqttClientFactory) {
        MqttPahoMessageHandler handler = new MqttPahoMessageHandler(clientId + "-out", mqttClientFactory);
        handler.setAsync(true);
        handler.setDefaultTopic("devices/broadcast/command");
        handler.setDefaultQos(1);
        return handler;
    }
}
