package iot;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;

@SpringBootApplication
@EnableScheduling
public class IoTCloudPlatformApplication {

    public static void main(String[] args) {
        SpringApplication.run(IoTCloudPlatformApplication.class, args);
    }
}
