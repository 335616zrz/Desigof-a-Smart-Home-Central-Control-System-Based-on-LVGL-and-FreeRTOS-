package iot.service;

import iot.domain.User;
import java.util.Optional;
import java.util.List;

public interface UserService {

    Optional<User> findByUsername(String username);

    Optional<User> findById(Long id);

    User register(String username, String rawPassword, String role, String email);

    List<User> list(int page, int pageSize);

    long count();

    User createUser(String username, String rawPassword, String role, String email);

    void deleteById(Long id);

    void deleteByIds(List<Long> ids);

    /**
     * Upgrade an operator to secondary admin
     * @param userId User to upgrade
     * @param upgradedBy Primary admin performing the upgrade
     * @return Updated user
     */
    User upgradeToSecondaryAdmin(Long userId, Long upgradedBy);

    /**
     * Downgrade a secondary admin to operator
     * @param userId User to downgrade
     * @param downgradedBy Primary admin performing the downgrade
     * @return Updated user
     */
    User downgradeToOperator(Long userId, Long downgradedBy);

    /**
     * Assign a user to a secondary admin manager
     * @param userId User to assign
     * @param managerId Secondary admin manager ID
     * @return Updated user
     */
    User assignUserToManager(Long userId, Long managerId);

    /**
     * Get all users managed by a secondary admin
     * @param managerId Manager ID
     * @return List of managed users
     */
    List<User> getUsersManagedBy(Long managerId);

    /**
     * Get secondary admins with available capacity (managing < 5 users)
     * @return List of secondary admins with space
     */
    List<User> getAvailableSecondaryAdmins();

    /**
     * Count users managed by a secondary admin
     * @param managerId Manager ID
     * @return Number of managed users
     */
    long countUsersManagedBy(Long managerId);
}
