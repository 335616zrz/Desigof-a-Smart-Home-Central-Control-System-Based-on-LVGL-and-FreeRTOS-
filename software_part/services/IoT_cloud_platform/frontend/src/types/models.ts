export interface User {
  id: number;
  username: string;
  email?: string;
  role: 'ROLE_PRIMARY_ADMIN' | 'ROLE_SECONDARY_ADMIN' | 'ROLE_ADMIN' | 'ROLE_OPERATOR' | 'ROLE_VIEWER' | string;
  createdAt?: string;
  status?: 'active' | 'disabled';
  isProtected?: boolean;
  managerId?: number;
}

export interface Device {
  id: string;
  deviceId: string;
  name: string;
  status: 'ONLINE' | 'OFFLINE' | 'UNKNOWN';
  firmwareVersion?: string;
  lastSeenAt?: string;
  createdAt?: string;
  tags?: string[];
  location?: string;
}

export interface DeviceCredential {
  clientId?: string;
  username?: string;
  password?: string;
  deviceId?: string;
  credentialKey?: string;
  credentialSecret?: string;
  expiresAt?: string;
}

export interface TelemetryRecord {
  id?: number;
  deviceId: string;
  payload?: string | Record<string, any>;
  reportedAt?: string;
  timestamp: number;
  temperature?: number;
  humidity?: number;
  pressure?: number;
  light?: number;
}

export interface AlertItem {
  id: number;
  deviceId: string;
  ruleId?: number;
  level: 'critical' | 'warning' | 'info';
  message: string;
  status: 'open' | 'ack' | 'closed';
  createdAt: string | number;
  acknowledgedAt?: string | number;
  closedAt?: string | number;
}

export interface Paginated<T> {
  items: T[];
  total: number;
}

export interface AlertRule {
  id?: number;
  name: string;
  description?: string;
  deviceId?: string | null;
  metric: string;
  operator: '>' | '<' | '>=' | '<=' | '==' | '!=';
  threshold: number;
  durationSeconds?: number | null;
  level: 'critical' | 'warning' | 'info';
  enabled: boolean;
  createdAt?: string;
  updatedAt?: string;

  season?: 'summer' | 'winter' | 'transition' | null;
  isSystemTemplate?: boolean;
  conditionType?: 'above' | 'below' | null;
}

export interface ServiceHealth {
  name: string;
  status: 'UP' | 'DOWN' | 'UNKNOWN';
  detail?: string;
}

export interface SeasonResponse {
  season: 'summer' | 'winter' | 'transition';
  seasonName: string;
}

// API Response Types
export interface ApiResponse<T = any> {
  code: number;
  message: string;
  data: T;
}

export interface ApiError {
  code: number;
  message: string;
  details?: string;
  timestamp?: number;
}

// Alert Type Alias
export type Alert = AlertItem;

// Severity Types
export type Severity = 'critical' | 'high' | 'medium' | 'low' | 'info';
export type AlertStatus = 'open' | 'acknowledged' | 'closed';
export type DeviceStatus = 'ONLINE' | 'OFFLINE' | 'UNKNOWN';
export type UserRole = 'ROLE_PRIMARY_ADMIN' | 'ROLE_SECONDARY_ADMIN' | 'ROLE_ADMIN' | 'ROLE_OPERATOR' | 'ROLE_VIEWER';

// Query Parameters
export interface PaginationParams {
  page?: number;
  pageSize?: number;
  sortBy?: string;
  sortOrder?: 'asc' | 'desc';
}

export interface DeviceQueryParams extends PaginationParams {
  keyword?: string;
  status?: DeviceStatus;
  tags?: string[];
}

export interface TelemetryQueryParams extends PaginationParams {
  deviceId?: string;
  metric?: string;
  from?: string;
  to?: string;
}

export interface AlertQueryParams extends PaginationParams {
  deviceId?: string;
  status?: AlertStatus;
  level?: Severity;
  from?: string;
  to?: string;
}

// Form Types
export interface LoginForm {
  username: string;
  password: string;
}

export interface DeviceForm {
  deviceId: string;
  name: string;
  firmwareVersion?: string;
  tags?: string[];
  location?: string;
}

export interface AlertRuleForm extends Omit<AlertRule, 'id' | 'createdAt' | 'updatedAt'> {
  id?: number;
}

// Store State Types
export interface DeviceState {
  items: Device[];
  loading: boolean;
  error: Error | null;
  total: number;
}

export interface TelemetryState {
  history: TelemetryRecord[];
  loading: boolean;
  error: Error | null;
  total: number;
}

export interface AlertState {
  items: AlertItem[];
  loading: boolean;
  error: Error | null;
  total: number;
}
