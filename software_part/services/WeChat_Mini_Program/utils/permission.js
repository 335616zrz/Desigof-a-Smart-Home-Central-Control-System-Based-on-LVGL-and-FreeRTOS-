import { USER_ROLES } from '../common/constants.js'

export function hasRole(user, role) {
  if (!user) return false
  const r = user.role
  if (r && r === role) return true
  const roles = user.roles
  if (Array.isArray(roles)) return roles.includes(role)
  return false
}

export function isAdmin(user) {
  return (
    hasRole(user, USER_ROLES.PRIMARY_ADMIN) ||
    hasRole(user, USER_ROLES.SECONDARY_ADMIN) ||
    hasRole(user, USER_ROLES.ADMIN)
  )
}
