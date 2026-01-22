package iot.controller;

import iot.domain.Alert;
import iot.service.AlertService;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.poi.ss.usermodel.Cell;
import org.apache.poi.ss.usermodel.CellStyle;
import org.apache.poi.ss.usermodel.Font;
import org.apache.poi.ss.usermodel.Row;
import org.apache.poi.ss.usermodel.Sheet;
import org.apache.poi.ss.usermodel.Workbook;
import org.apache.poi.xssf.usermodel.XSSFWorkbook;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/alerts")
public class AlertController {

    private final AlertService alertService;

    public AlertController(AlertService alertService) {
        this.alertService = alertService;
    }

    @GetMapping
    public ResponseEntity<Map<String, Object>> listAlerts(
            @RequestParam(required = false) String status,
            @RequestParam(required = false) String level,
            @RequestParam(required = false, defaultValue = "0") Integer page,
            @RequestParam(required = false, defaultValue = "20") Integer pageSize) {

        List<Alert> items = alertService.listAlerts(status, level, page, pageSize);
        Long total = alertService.countAlerts(status, level);

        Map<String, Object> response = new HashMap<>();
        response.put("items", items);
        response.put("total", total);
        response.put("page", page);
        response.put("pageSize", pageSize);

        return ResponseEntity.ok(response);
    }

    @GetMapping("/{id}")
    public ResponseEntity<Alert> getAlert(@PathVariable Long id) {
        return alertService.findById(id)
                .map(ResponseEntity::ok)
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    @GetMapping("/device/{deviceId}")
    public List<Alert> getDeviceAlerts(@PathVariable String deviceId) {
        return alertService.findByDeviceId(deviceId);
    }

    @PostMapping
    public ResponseEntity<Alert> createAlert(@RequestBody Alert alert) {
        Alert created = alertService.createAlert(alert);
        return ResponseEntity.status(HttpStatus.CREATED).body(created);
    }

    @PostMapping("/{id}/ack")
    public ResponseEntity<Void> acknowledgeAlert(@PathVariable Long id) {
        alertService.acknowledge(id);
        return ResponseEntity.ok().build();
    }

    @PostMapping("/{id}/close")
    public ResponseEntity<Void> closeAlert(@PathVariable Long id) {
        alertService.close(id);
        return ResponseEntity.ok().build();
    }

    @DeleteMapping("/{id}")
    public ResponseEntity<Void> deleteAlert(@PathVariable Long id) {
        alertService.deleteAlert(id);
        return ResponseEntity.noContent().build();
    }

    @GetMapping("/export")
    @PreAuthorize("hasAnyAuthority('ROLE_ADMIN','ROLE_PRIMARY_ADMIN','ROLE_SECONDARY_ADMIN','ROLE_OPERATOR')")
    public void exportAlerts(
            @RequestParam(required = false) String status,
            @RequestParam(required = false) String level,
            @RequestParam(required = false) String deviceId,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime from,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime to,
            HttpServletResponse response) throws IOException {
        List<Alert> alerts = alertService.listAlertsForExport(status, level, deviceId, from, to);

        Workbook workbook = new XSSFWorkbook();
        Sheet sheet = workbook.createSheet("告警记录");

        CellStyle headerStyle = workbook.createCellStyle();
        Font headerFont = workbook.createFont();
        headerFont.setBold(true);
        headerStyle.setFont(headerFont);

        String[] headers = {"ID", "设备ID", "规则ID", "告警级别", "告警消息", "状态", "创建时间", "确认时间", "关闭时间"};
        Row headerRow = sheet.createRow(0);
        for (int i = 0; i < headers.length; i++) {
            Cell cell = headerRow.createCell(i);
            cell.setCellValue(headers[i]);
            cell.setCellStyle(headerStyle);
        }

        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        int rowNum = 1;
        for (Alert alert : alerts) {
            Row row = sheet.createRow(rowNum++);
            row.createCell(0).setCellValue(alert.getId() == null ? 0 : alert.getId());
            row.createCell(1).setCellValue(alert.getDeviceId());
            row.createCell(2).setCellValue(alert.getRuleId() == null ? "" : String.valueOf(alert.getRuleId()));
            row.createCell(3).setCellValue(getLevelDisplayName(alert.getLevel()));
            row.createCell(4).setCellValue(alert.getMessage());
            row.createCell(5).setCellValue(getStatusDisplayName(alert.getStatus()));
            row.createCell(6).setCellValue(formatDateTime(alert.getCreatedAt(), formatter));
            row.createCell(7).setCellValue(formatDateTime(alert.getAcknowledgedAt(), formatter));
            row.createCell(8).setCellValue(formatDateTime(alert.getClosedAt(), formatter));
        }

        for (int i = 0; i < headers.length; i++) {
            sheet.autoSizeColumn(i);
            if (sheet.getColumnWidth(i) < 3000) {
                sheet.setColumnWidth(i, 3000);
            }
        }

        String filename = "alerts_" + LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyyMMdd_HHmmss")) + ".xlsx";
        String encodedFilename = URLEncoder.encode(filename, StandardCharsets.UTF_8).replaceAll("\\+", "%20");

        response.setContentType("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
        response.setHeader("Content-Disposition", "attachment; filename*=UTF-8''" + encodedFilename);

        workbook.write(response.getOutputStream());
        workbook.close();
    }

    private String formatDateTime(LocalDateTime value, DateTimeFormatter formatter) {
        if (value == null) {
            return "";
        }
        return value.format(formatter);
    }

    private String getLevelDisplayName(String level) {
        if (level == null) {
            return "";
        }
        return switch (level) {
            case "critical" -> "危险";
            case "warning" -> "警告";
            case "info" -> "提示";
            default -> level;
        };
    }

    private String getStatusDisplayName(String status) {
        if (status == null) {
            return "";
        }
        return switch (status) {
            case "open" -> "未处理";
            case "ack" -> "已确认";
            case "closed" -> "已关闭";
            default -> status;
        };
    }
}
