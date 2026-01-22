import { describe, it, expect, vi } from 'vitest';
import { formatDateTime, formatRelative } from './format';

describe('format utilities', () => {
  it('formats datetime', () => {
    const ts = Date.UTC(2024, 5, 30, 0, 0, 0); // 2024-06-30 00:00:00 UTC
    expect(formatDateTime(ts)).toMatch(/2024-06-30/);
  });

  it('handles empty date', () => {
    expect(formatDateTime(undefined)).toBe('-');
  });

  it('formats relative within minute', () => {
    vi.useFakeTimers();
    vi.setSystemTime(new Date('2025-01-01T00:00:30Z'));
    expect(formatRelative(new Date('2025-01-01T00:00:10Z').getTime())).toBe('刚刚');
    vi.useRealTimers();
  });
});
