package iot.controller;

import iot.domain.User;
import iot.dto.ApiResponse;
import iot.dto.AuthRequest;
import iot.dto.AuthResponse;
import iot.dto.RegisterRequest;
import iot.dto.UserResponse;
import iot.security.JwtTokenProvider;
import iot.service.UserService;
import jakarta.validation.Valid;
import java.util.Map;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.authentication.BadCredentialsException;
import org.springframework.security.core.Authentication;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/auth")
public class AuthController {

    private final JwtTokenProvider jwtTokenProvider;
    private final UserService userService;
    private final PasswordEncoder passwordEncoder;

    public AuthController(JwtTokenProvider jwtTokenProvider,
                          UserService userService,
                          PasswordEncoder passwordEncoder) {
        this.jwtTokenProvider = jwtTokenProvider;
        this.userService = userService;
        this.passwordEncoder = passwordEncoder;
    }

    @PostMapping("/login")
    public ResponseEntity<ApiResponse<AuthResponse>> login(@Valid @RequestBody AuthRequest request) {
        User user = userService.findByUsername(request.getUsername())
                .orElseThrow(() -> new BadCredentialsException("Invalid credentials"));
        if (!passwordEncoder.matches(request.getPassword(), user.getPasswordHash())) {
            throw new BadCredentialsException("Invalid credentials");
        }
        String access = jwtTokenProvider.generateAccessToken(user.getUsername(), user.getRole());
        String refresh = jwtTokenProvider.generateRefreshToken(user.getUsername(), user.getRole());
        return ResponseEntity.ok(ApiResponse.ok(new AuthResponse(access, refresh, user.getUsername(), user.getRole())));
    }

    @PostMapping("/register")
    public ResponseEntity<ApiResponse<AuthResponse>> register(@Valid @RequestBody RegisterRequest request) {
        if (userService.findByUsername(request.getUsername()).isPresent()) {
            return ResponseEntity.status(HttpStatus.CONFLICT)
                    .body(ApiResponse.error(HttpStatus.CONFLICT.value(), "用户名已存在"));
        }
        // 新注册用户默认为 ROLE_OPERATOR（可管理自己的设备）
        User user = userService.register(request.getUsername(), request.getPassword(), "ROLE_OPERATOR", request.getEmail());
        String access = jwtTokenProvider.generateAccessToken(user.getUsername(), user.getRole());
        String refresh = jwtTokenProvider.generateRefreshToken(user.getUsername(), user.getRole());
        return ResponseEntity.status(HttpStatus.CREATED)
                .body(ApiResponse.ok(new AuthResponse(access, refresh, user.getUsername(), user.getRole())));
    }

    @PostMapping("/refresh")
    public ResponseEntity<ApiResponse<AuthResponse>> refresh(@RequestBody Map<String, String> payload) {
        String refreshToken = payload.get("refreshToken");
        if (refreshToken == null || !jwtTokenProvider.validateToken(refreshToken)) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body(ApiResponse.error(401, "Refresh token invalid"));
        }
        var claims = jwtTokenProvider.parseClaims(refreshToken);
        Object type = claims.get("type");
        if (type == null || !"refresh".equals(type.toString())) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body(ApiResponse.error(401, "Not a refresh token"));
        }
        String username = claims.getSubject();
        String role = claims.get("role", String.class);
        String access = jwtTokenProvider.generateAccessToken(username, role);
        String newRefresh = jwtTokenProvider.generateRefreshToken(username, role);
        return ResponseEntity.ok(ApiResponse.ok(new AuthResponse(access, newRefresh, username, role)));
    }

    @PostMapping("/profile")
    public ResponseEntity<ApiResponse<UserResponse>> profile(Authentication authentication) {
        if (authentication == null || authentication.getName() == null) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED).body(ApiResponse.error(401, "Unauthorized"));
        }
        User user = userService.findByUsername(authentication.getName())
                .orElseThrow(() -> new BadCredentialsException("User not found"));
        UserResponse resp = new UserResponse();
        resp.setId(user.getId());
        resp.setUsername(user.getUsername());
        resp.setEmail(user.getEmail());
        resp.setRole(user.getRole());
        resp.setIsProtected(user.getIsProtected());
        resp.setManagerId(user.getManagerId());
        resp.setCreatedAt(user.getCreatedAt());
        return ResponseEntity.ok(ApiResponse.ok(resp));
    }
}
