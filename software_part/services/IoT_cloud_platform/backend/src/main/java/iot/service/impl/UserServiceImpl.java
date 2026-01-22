package iot.service.impl;

import iot.domain.User;
import iot.mapper.UserMapper;
import iot.service.UserService;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@Transactional(readOnly = true)
public class UserServiceImpl implements UserService {

    private final UserMapper userMapper;
    private final PasswordEncoder passwordEncoder;

    public UserServiceImpl(UserMapper userMapper, PasswordEncoder passwordEncoder) {
        this.userMapper = userMapper;
        this.passwordEncoder = passwordEncoder;
    }

    @Override
    public Optional<User> findByUsername(String username) {
        return Optional.ofNullable(userMapper.findByUsername(username));
    }

    @Override
    public Optional<User> findById(Long id) {
        return Optional.ofNullable(userMapper.findById(id));
    }

    @Override
    @Transactional
    public User register(String username, String rawPassword, String role, String email) {
        User user = new User();
        user.setUsername(username);
        user.setPasswordHash(passwordEncoder.encode(rawPassword));
        user.setRole(role);
        user.setEmail(email);
        user.setIsProtected(false);
        user.setCreatedAt(LocalDateTime.now());
        userMapper.insert(user);
        return user;
    }

    @Override
    public List<User> list(int page, int pageSize) {
        int limit = Math.max(pageSize, 1);
        int offset = Math.max(page - 1, 0) * limit;
        return userMapper.findAll(offset, limit);
    }

    @Override
    public long count() {
        return userMapper.countAll();
    }

    @Override
    @Transactional
    public User createUser(String username, String rawPassword, String role, String email) {
        User user = new User();
        user.setUsername(username);
        user.setPasswordHash(passwordEncoder.encode(rawPassword));
        user.setRole(role);
        user.setEmail(email);
        user.setIsProtected(false);
        user.setCreatedAt(LocalDateTime.now());
        userMapper.insert(user);
        return user;
    }

    @Override
    @Transactional
    public void deleteById(Long id) {
        // Fetch the user to be deleted
        User userToDelete = userMapper.findById(id);
        if (userToDelete == null) {
            throw new RuntimeException("User not found with id: " + id);
        }

        // Protection: Cannot delete protected users
        if (Boolean.TRUE.equals(userToDelete.getIsProtected())) {
            throw new RuntimeException("Cannot delete protected user account");
        }

        // Protection: Cannot delete a ROLE_ADMIN or ROLE_PRIMARY_ADMIN if it's the last one
        if ("ROLE_ADMIN".equals(userToDelete.getRole()) || "ROLE_PRIMARY_ADMIN".equals(userToDelete.getRole())) {
            long adminCount = userMapper.countByRole("ROLE_PRIMARY_ADMIN");
            if (adminCount <= 1) {
                throw new RuntimeException("Cannot delete the last administrator account");
            }
        }

        userMapper.deleteById(id);
    }

    @Override
    @Transactional
    public void deleteByIds(List<Long> ids) {
        if (ids == null || ids.isEmpty()) return;

        // Check for protected users and count admins to be deleted
        long adminToDeleteCount = 0;
        for (Long id : ids) {
            User user = userMapper.findById(id);
            if (user != null) {
                // Check if user is protected
                if (Boolean.TRUE.equals(user.getIsProtected())) {
                    throw new RuntimeException("Cannot delete protected user account: " + user.getUsername());
                }
                // Count admins
                if ("ROLE_ADMIN".equals(user.getRole()) || "ROLE_PRIMARY_ADMIN".equals(user.getRole())) {
                    adminToDeleteCount++;
                }
            }
        }

        // Check if deletion would remove all admins
        if (adminToDeleteCount > 0) {
            long currentAdminCount = userMapper.countByRole("ROLE_PRIMARY_ADMIN");
            if (currentAdminCount - adminToDeleteCount < 1) {
                throw new RuntimeException("Cannot delete all administrator accounts. At least one admin must remain.");
            }
        }

        userMapper.deleteByIds(ids);
    }

    @Override
    @Transactional
    public User upgradeToSecondaryAdmin(Long userId, Long upgradedBy) {
        // Validate user exists
        User user = userMapper.findById(userId);
        if (user == null) {
            throw new RuntimeException("User not found with id: " + userId);
        }

        // Validate performer is primary admin
        User upgrader = userMapper.findById(upgradedBy);
        if (upgrader == null || !"ROLE_PRIMARY_ADMIN".equals(upgrader.getRole())) {
            throw new RuntimeException("Only primary admins can upgrade users");
        }

        // Validate user is currently an operator
        if (!"ROLE_OPERATOR".equals(user.getRole())) {
            throw new RuntimeException("Can only upgrade operators to secondary admin");
        }

        // Clear manager_id when upgrading to secondary admin
        userMapper.updateManagerId(userId, null);
        userMapper.updateRole(userId, "ROLE_SECONDARY_ADMIN");

        // Return updated user
        return userMapper.findById(userId);
    }

    @Override
    @Transactional
    public User downgradeToOperator(Long userId, Long downgradedBy) {
        // Validate user exists
        User user = userMapper.findById(userId);
        if (user == null) {
            throw new RuntimeException("User not found with id: " + userId);
        }

        // Validate performer is primary admin
        User downgrader = userMapper.findById(downgradedBy);
        if (downgrader == null || !"ROLE_PRIMARY_ADMIN".equals(downgrader.getRole())) {
            throw new RuntimeException("Only primary admins can downgrade users");
        }

        // Validate user is currently a secondary admin
        if (!"ROLE_SECONDARY_ADMIN".equals(user.getRole())) {
            throw new RuntimeException("Can only downgrade secondary admins to operator");
        }

        // Check if this secondary admin is managing any users
        long managedCount = userMapper.countByManagerId(userId);
        if (managedCount > 0) {
            throw new RuntimeException("Cannot downgrade secondary admin who is managing " + managedCount + " users. Reassign users first.");
        }

        // Downgrade to operator (manager_id will remain null)
        userMapper.updateRole(userId, "ROLE_OPERATOR");

        // Return updated user
        return userMapper.findById(userId);
    }

    @Override
    @Transactional
    public User assignUserToManager(Long userId, Long managerId) {
        // Validate user exists
        User user = userMapper.findById(userId);
        if (user == null) {
            throw new RuntimeException("User not found with id: " + userId);
        }

        // Validate user is an operator
        if (!"ROLE_OPERATOR".equals(user.getRole())) {
            throw new RuntimeException("Can only assign operators to managers");
        }

        // Validate manager exists and is a secondary admin
        User manager = userMapper.findById(managerId);
        if (manager == null) {
            throw new RuntimeException("Manager not found with id: " + managerId);
        }
        if (!"ROLE_SECONDARY_ADMIN".equals(manager.getRole())) {
            throw new RuntimeException("Manager must be a secondary admin");
        }

        // Check manager's current group size
        long currentGroupSize = userMapper.countByManagerId(managerId);
        if (currentGroupSize >= 5) {
            throw new RuntimeException("Manager has reached maximum group size of 5 users");
        }

        // Assign user to manager
        userMapper.updateManagerId(userId, managerId);

        // Return updated user
        return userMapper.findById(userId);
    }

    @Override
    public List<User> getUsersManagedBy(Long managerId) {
        // Validate manager exists
        User manager = userMapper.findById(managerId);
        if (manager == null) {
            throw new RuntimeException("Manager not found with id: " + managerId);
        }

        return userMapper.findByManagerId(managerId);
    }

    @Override
    public List<User> getAvailableSecondaryAdmins() {
        return userMapper.findSecondaryAdminsWithSpace(5);
    }

    @Override
    public long countUsersManagedBy(Long managerId) {
        return userMapper.countByManagerId(managerId);
    }
}
