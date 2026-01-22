// Shared constants used across the mini program.

export const STORAGE_KEYS = {
  TOKEN: 'W_TOKEN',
  REFRESH_TOKEN: 'W_REFRESH_TOKEN',
  USER: 'W_USER',
  LAST_LOGIN_AT: 'W_LAST_LOGIN_AT',
}

export const HTTP = {
  AUTH_HEADER: 'Authorization',
  TOKEN_PREFIX: 'Bearer ',
}

export const ROUTES = {
  INDEX: '/pages/index/index',
  LOGIN: '/pages/login/login',
  REGISTER: '/pages/register/register',
}

export const USER_ROLES = {
  PRIMARY_ADMIN: 'ROLE_PRIMARY_ADMIN',
  SECONDARY_ADMIN: 'ROLE_SECONDARY_ADMIN',
  ADMIN: 'ROLE_ADMIN',
  OPERATOR: 'ROLE_OPERATOR',
}
