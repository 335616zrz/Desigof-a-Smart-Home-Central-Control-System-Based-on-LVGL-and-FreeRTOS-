export interface ApiResponse<T> {
  code?: number;
  message?: string;
  data: T;
}

export interface AuthTokens {
  accessToken: string;
  refreshToken?: string;
}
