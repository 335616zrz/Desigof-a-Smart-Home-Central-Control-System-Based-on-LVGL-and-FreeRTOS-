import * as XLSX from 'xlsx';
import dayjs from 'dayjs';
import type { TelemetryRecord } from '@/types/models';

/**
 * 导出遥测数据到Excel
 */
export function exportTelemetryToExcel(data: TelemetryRecord[], filename?: string) {
  if (!data || data.length === 0) {
    throw new Error('没有数据可导出');
  }

  // 转换数据为表格格式
  const rows = data.map(record => ({
    设备ID: record.deviceId,
    时间戳: dayjs(record.timestamp).format('YYYY-MM-DD HH:mm:ss'),
    温度: record.temperature != null ? record.temperature : '',
    湿度: record.humidity != null ? record.humidity : '',
    气压: record.pressure != null ? record.pressure : '',
    光照: record.light != null ? record.light : ''
  }));

  // 创建工作簿
  const ws = XLSX.utils.json_to_sheet(rows);
  const wb = XLSX.utils.book_new();
  XLSX.utils.book_append_sheet(wb, ws, '遥测数据');

  // 设置列宽
  ws['!cols'] = [
    { wch: 25 }, // 设备ID
    { wch: 20 }, // 时间戳
    { wch: 10 }, // 温度
    { wch: 10 }, // 湿度
    { wch: 10 }, // 气压
    { wch: 10 }  // 光照
  ];

  // 下载文件
  const defaultFilename = `telemetry_${dayjs().format('YYYYMMDD_HHmmss')}.xlsx`;
  XLSX.writeFile(wb, filename || defaultFilename);
}

/**
 * 导出遥测数据到CSV
 */
export function exportTelemetryToCSV(data: TelemetryRecord[], filename?: string) {
  if (!data || data.length === 0) {
    throw new Error('没有数据可导出');
  }

  // 转换数据为表格格式
  const rows = data.map(record => ({
    设备ID: record.deviceId,
    时间戳: dayjs(record.timestamp).format('YYYY-MM-DD HH:mm:ss'),
    温度: record.temperature != null ? record.temperature : '',
    湿度: record.humidity != null ? record.humidity : '',
    气压: record.pressure != null ? record.pressure : '',
    光照: record.light != null ? record.light : ''
  }));

  // 创建工作簿
  const ws = XLSX.utils.json_to_sheet(rows);
  const csv = XLSX.utils.sheet_to_csv(ws);

  // 下载文件
  const defaultFilename = `telemetry_${dayjs().format('YYYYMMDD_HHmmss')}.csv`;
  const blob = new Blob(['\ufeff' + csv], { type: 'text/csv;charset=utf-8;' }); // \ufeff 是BOM，确保Excel正确识别UTF-8
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = filename || defaultFilename;
  link.click();
  URL.revokeObjectURL(link.href);
}

/**
 * 导出JSON数据
 */
export function exportToJSON<T>(data: T, filename?: string) {
  const json = JSON.stringify(data, null, 2);
  const defaultFilename = `export_${dayjs().format('YYYYMMDD_HHmmss')}.json`;
  const blob = new Blob([json], { type: 'application/json' });
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = filename || defaultFilename;
  link.click();
  URL.revokeObjectURL(link.href);
}
