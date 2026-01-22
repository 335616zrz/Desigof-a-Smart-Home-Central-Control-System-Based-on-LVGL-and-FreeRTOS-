package iot.security;

import iot.service.UserService;
import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Collections;
import org.springframework.http.HttpHeaders;
import org.springframework.security.authentication.UsernamePasswordAuthenticationToken;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.web.authentication.WebAuthenticationDetailsSource;
import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;
import org.springframework.web.filter.OncePerRequestFilter;

@Component
public class JwtAuthenticationFilter extends OncePerRequestFilter {

    private final JwtTokenProvider jwtTokenProvider;
    private final UserService userService;

    public JwtAuthenticationFilter(JwtTokenProvider jwtTokenProvider, UserService userService) {
        this.jwtTokenProvider = jwtTokenProvider;
        this.userService = userService;
    }

    @Override
    protected void doFilterInternal(HttpServletRequest request, HttpServletResponse response, FilterChain filterChain)
            throws ServletException, IOException {
        String authHeader = request.getHeader(HttpHeaders.AUTHORIZATION);
        if (StringUtils.hasText(authHeader) && authHeader.startsWith("Bearer ")) {
            String token = authHeader.substring(7);
            if (jwtTokenProvider.validateToken(token)) {
                String username = jwtTokenProvider.parseClaims(token).getSubject();
                userService.findByUsername(username).ifPresent(user -> {
                    System.out.println("=== JWT Auth Filter DEBUG ===");
                    System.out.println("Username: " + user.getUsername());
                    System.out.println("Role: " + user.getRole());
                    // Normalize role so both "ADMIN" and "ROLE_ADMIN" work consistently.
                    SimpleGrantedAuthority authority = new SimpleGrantedAuthority(RoleUtils.normalize(user.getRole()));
                    System.out.println("Created authority: " + authority);
                    Authentication authentication = new UsernamePasswordAuthenticationToken(
                            user.getUsername(), null, Collections.singletonList(authority));
                    ((UsernamePasswordAuthenticationToken) authentication)
                            .setDetails(new WebAuthenticationDetailsSource().buildDetails(request));
                    SecurityContextHolder.getContext().setAuthentication(authentication);
                    System.out.println("Authentication authorities: " + authentication.getAuthorities());
                });
            }
        }
        filterChain.doFilter(request, response);
    }
}
