package iot.security;

import iot.config.JwtProperties;
import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.SignatureAlgorithm;
import java.nio.charset.StandardCharsets;
import java.util.Date;
import org.springframework.stereotype.Component;

@Component
public class JwtTokenProvider {

    private final JwtProperties jwtProperties;

    public JwtTokenProvider(JwtProperties jwtProperties) {
        this.jwtProperties = jwtProperties;
    }

    public String generateAccessToken(String username, String role) {
        return buildToken(username, role, jwtProperties.getExpirationSeconds(), "access");
    }

    public String generateRefreshToken(String username, String role) {
        return buildToken(username, role, jwtProperties.getRefreshExpirationSeconds(), "refresh");
    }

    public boolean validateToken(String token) {
        try {
            Jwts.parserBuilder()
                    .setSigningKey(jwtProperties.getSecret().getBytes(StandardCharsets.UTF_8))
                    .build()
                    .parseClaimsJws(token);
            return true;
        } catch (Exception ex) {
            return false;
        }
    }

    public Claims parseClaims(String token) {
        return Jwts.parserBuilder()
                .setSigningKey(jwtProperties.getSecret().getBytes(StandardCharsets.UTF_8))
                .build()
                .parseClaimsJws(token)
                .getBody();
    }

    private String buildToken(String username, String role, long expirySeconds, String type) {
        Date now = new Date();
        Date expiration = new Date(now.getTime() + expirySeconds * 1000);
        return Jwts.builder()
                .setSubject(username)
                .claim("role", role)
                .claim("type", type)
                .setIssuedAt(now)
                .setExpiration(expiration)
                .signWith(SignatureAlgorithm.HS256, jwtProperties.getSecret().getBytes(StandardCharsets.UTF_8))
                .compact();
    }
}
