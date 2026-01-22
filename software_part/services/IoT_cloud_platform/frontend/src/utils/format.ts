import dayjs from 'dayjs';

export function formatDateTime(value?: string | number) {
  if (!value) return '-';
  return dayjs(value).format('YYYY-MM-DD HH:mm:ss');
}

// 用于后端 LocalDateTime 查询参数（不要用 Date.toISOString()，它会转成UTC导致偏移）
export function toLocalIsoDateTime(value: Date | string | number) {
  return dayjs(value).format('YYYY-MM-DDTHH:mm:ss');
}

export function formatRelative(value?: string | number) {
  if (!value) return '-';
  const diff = Date.now() - dayjs(value).valueOf();
  if (diff < 60 * 1000) return '刚刚';
  if (diff < 3600 * 1000) return `${Math.floor(diff / 60000)} 分钟前`;
  if (diff < 24 * 3600 * 1000) return `${Math.floor(diff / 3600000)} 小时前`;
  return dayjs(value).format('MM-DD HH:mm');
}
