package iot.dto;

public class AuthResponse {

    private String accessToken;
    private String refreshToken;
    private String username;
    private String role;

    public AuthResponse(String accessToken, String refreshToken, String username, String role) {
        this.accessToken = accessToken;
        this.refreshToken = refreshToken;
        this.username = username;
        this.role = role;
    }

    public String getAccessToken() {
        return accessToken;
    }

    public String getRefreshToken() {
        return refreshToken;
    }

    public String getUsername() {
        return username;
    }

    public String getRole() {
        return role;
    }
}
