package iot.security;

/**
 * Role helper utilities.
 *
 * <p>Historical note: some deployments stored roles as "ADMIN"/"PRIMARY_ADMIN" without the "ROLE_" prefix.
 * Spring Security authorities typically use the "ROLE_" prefix. This utility normalizes roles so the backend
 * remains backward-compatible with existing databases.
 */
public final class RoleUtils {

    private RoleUtils() {}

    /**
     * Normalize a role string to the Spring Security authority format: "ROLE_XXX".
     * Returns null when input is null.
     */
    public static String normalize(String role) {
        if (role == null) return null;
        String r = role.trim();
        if (r.isEmpty()) return r;
        r = r.toUpperCase();
        if (r.startsWith("ROLE_")) return r;
        return "ROLE_" + r;
    }

    public static boolean isAdminRole(String role) {
        String r = normalize(role);
        return "ROLE_PRIMARY_ADMIN".equals(r) ||
                "ROLE_SECONDARY_ADMIN".equals(r) ||
                "ROLE_ADMIN".equals(r);
    }
}

