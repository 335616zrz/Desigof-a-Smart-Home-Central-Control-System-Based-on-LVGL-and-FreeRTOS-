package iot.controller;

import iot.domain.User;
import iot.dto.ManagerAssignmentRequest;
import iot.dto.UserCreateRequest;
import iot.dto.UserResponse;
import iot.service.UserService;
import jakarta.validation.Valid;
import java.util.List;
import java.util.stream.Collectors;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/users")
public class UserController {

    private final UserService userService;

    public UserController(UserService userService) {
        this.userService = userService;
    }

    @GetMapping
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN', 'ROLE_SECONDARY_ADMIN', 'ROLE_ADMIN')")
    public ResponseEntity<?> list(
            @RequestParam(defaultValue = "1") int page,
            @RequestParam(defaultValue = "20") int pageSize) {
        List<UserResponse> users = userService.list(page, pageSize).stream().map(this::toResponse).collect(Collectors.toList());
        long total = userService.count();
        return ResponseEntity.ok(new Paginated<>(users, total));
    }

    @PostMapping
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN', 'ROLE_ADMIN')")
    public ResponseEntity<UserResponse> create(@Valid @RequestBody UserCreateRequest req) {
        User user = userService.createUser(req.getUsername(), req.getPassword(), req.getRole(), req.getEmail());
        return ResponseEntity.status(HttpStatus.CREATED).body(toResponse(user));
    }

    @PostMapping("/batch-create")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN', 'ROLE_ADMIN')")
    public ResponseEntity<List<UserResponse>> batchCreate(@Valid @RequestBody List<UserCreateRequest> requests) {
        List<UserResponse> created = requests.stream()
                .map(req -> userService.createUser(req.getUsername(), req.getPassword(), req.getRole(), req.getEmail()))
                .map(this::toResponse)
                .toList();
        return ResponseEntity.status(HttpStatus.CREATED).body(created);
    }

    @DeleteMapping("/{id}")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN', 'ROLE_ADMIN')")
    public ResponseEntity<Void> delete(@PathVariable Long id) {
        // Get current authenticated user
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();

        // Get target user to be deleted
        User targetUser = userService.findById(id).orElse(null);
        if (targetUser == null) {
            return ResponseEntity.notFound().build();
        }

        // Protect users marked as protected
        if (Boolean.TRUE.equals(targetUser.getIsProtected())) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        // Check if trying to delete self
        User currentUser = userService.findByUsername(currentUsername).orElse(null);
        if (currentUser != null && currentUser.getId().equals(id)) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        userService.deleteById(id);
        return ResponseEntity.noContent().build();
    }

    @PostMapping("/batch-delete")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN', 'ROLE_ADMIN')")
    public ResponseEntity<Void> batchDelete(@RequestBody List<Long> ids) {
        // Get current authenticated user
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();

        // Check if trying to delete protected users
        for (Long id : ids) {
            User targetUser = userService.findById(id).orElse(null);
            if (targetUser != null && Boolean.TRUE.equals(targetUser.getIsProtected())) {
                return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
            }
        }

        // Check if trying to delete self
        User currentUser = userService.findByUsername(currentUsername).orElse(null);
        if (currentUser != null && ids.contains(currentUser.getId())) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        userService.deleteByIds(ids);
        return ResponseEntity.noContent().build();
    }

    /**
     * Upgrade an operator to secondary admin (Primary admin only)
     *
     * @param id User ID to upgrade
     * @return Updated user response
     */
    @PostMapping("/{id}/upgrade")
    @PreAuthorize("hasAuthority('ROLE_PRIMARY_ADMIN')")
    public ResponseEntity<UserResponse> upgradeToSecondaryAdmin(@PathVariable Long id) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();
        User currentUser = userService.findByUsername(currentUsername).orElse(null);

        if (currentUser == null) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        User upgradedUser = userService.upgradeToSecondaryAdmin(id, currentUser.getId());
        return ResponseEntity.ok(toResponse(upgradedUser));
    }

    /**
     * Downgrade a secondary admin to operator (Primary admin only)
     *
     * @param id User ID to downgrade
     * @return Updated user response
     */
    @PostMapping("/{id}/downgrade")
    @PreAuthorize("hasAuthority('ROLE_PRIMARY_ADMIN')")
    public ResponseEntity<UserResponse> downgradeToOperator(@PathVariable Long id) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();
        User currentUser = userService.findByUsername(currentUsername).orElse(null);

        if (currentUser == null) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        User downgradedUser = userService.downgradeToOperator(id, currentUser.getId());
        return ResponseEntity.ok(toResponse(downgradedUser));
    }

    /**
     * Assign a manager to a user (Primary admin only)
     *
     * @param id User ID to assign manager to
     * @param request Manager assignment request containing managerId
     * @return Updated user response
     */
    @PostMapping("/{id}/assign-manager")
    @PreAuthorize("hasAuthority('ROLE_PRIMARY_ADMIN')")
    public ResponseEntity<UserResponse> assignManager(
            @PathVariable Long id,
            @Valid @RequestBody ManagerAssignmentRequest request) {
        User updatedUser = userService.assignUserToManager(id, request.getManagerId());
        return ResponseEntity.ok(toResponse(updatedUser));
    }

    /**
     * Get users managed by a specific secondary admin
     *
     * @param id Manager user ID
     * @return List of managed users
     */
    @GetMapping("/{id}/managed-users")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN', 'ROLE_SECONDARY_ADMIN', 'ROLE_ADMIN')")
    public ResponseEntity<List<UserResponse>> getManagedUsers(@PathVariable Long id) {
        List<UserResponse> managedUsers = userService.getUsersManagedBy(id)
                .stream()
                .map(this::toResponse)
                .collect(Collectors.toList());
        return ResponseEntity.ok(managedUsers);
    }

    /**
     * Get available secondary admins with capacity (managing < 5 users)
     *
     * @return List of available secondary admins
     */
    @GetMapping("/available-managers")
    @PreAuthorize("hasAuthority('ROLE_PRIMARY_ADMIN')")
    public ResponseEntity<List<UserResponse>> getAvailableManagers() {
        List<UserResponse> availableManagers = userService.getAvailableSecondaryAdmins()
                .stream()
                .map(this::toResponse)
                .collect(Collectors.toList());
        return ResponseEntity.ok(availableManagers);
    }

    /**
     * Count users managed by a specific secondary admin
     *
     * @param id Manager user ID
     * @return Count of managed users
     */
    @GetMapping("/{id}/managed-count")
    @PreAuthorize("hasAnyAuthority('ROLE_PRIMARY_ADMIN', 'ROLE_SECONDARY_ADMIN', 'ROLE_ADMIN')")
    public ResponseEntity<Long> getManagedCount(@PathVariable Long id) {
        long count = userService.countUsersManagedBy(id);
        return ResponseEntity.ok(count);
    }

    private UserResponse toResponse(User user) {
        UserResponse resp = new UserResponse();
        resp.setId(user.getId());
        resp.setUsername(user.getUsername());
        resp.setEmail(user.getEmail());
        resp.setRole(user.getRole());
        resp.setIsProtected(user.getIsProtected());
        resp.setManagerId(user.getManagerId());
        resp.setCreatedAt(user.getCreatedAt());
        return resp;
    }

    private record Paginated<T>(List<T> items, long total) {}
}
