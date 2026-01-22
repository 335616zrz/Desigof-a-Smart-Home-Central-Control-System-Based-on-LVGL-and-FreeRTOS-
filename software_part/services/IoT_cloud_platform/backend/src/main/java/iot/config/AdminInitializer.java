package iot.config;

import iot.mapper.UserMapper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.CommandLineRunner;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Component;

@Component
public class AdminInitializer implements CommandLineRunner {
    private static final Logger log = LoggerFactory.getLogger(AdminInitializer.class);

    private final UserMapper userMapper;
    private final PasswordEncoder passwordEncoder;

    @Value("${app.admin.username:}")
    private String adminUsername;

    @Value("${app.admin.password:}")
    private String adminPassword;

    @Value("${app.admin.email:admin@example.com}")
    private String adminEmail;

    public AdminInitializer(UserMapper userMapper, PasswordEncoder passwordEncoder) {
        this.userMapper = userMapper;
        this.passwordEncoder = passwordEncoder;
    }

    @Override
    public void run(String... args) {
        if (adminUsername == null || adminUsername.isBlank() || adminPassword == null || adminPassword.isBlank()) {
            log.warn("AdminInitializer disabled: set app.admin.username/app.admin.password (or env APP_ADMIN_USERNAME/APP_ADMIN_PASSWORD) to bootstrap an admin user.");
            return;
        }

        String email = (adminEmail == null || adminEmail.isBlank()) ? "admin@example.com" : adminEmail;
        String hash = passwordEncoder.encode(adminPassword);
        userMapper.upsertAdmin(adminUsername, hash, email, "ROLE_PRIMARY_ADMIN");
        log.info("AdminInitializer ensured primary admin user '{}'.", adminUsername);
    }
}
