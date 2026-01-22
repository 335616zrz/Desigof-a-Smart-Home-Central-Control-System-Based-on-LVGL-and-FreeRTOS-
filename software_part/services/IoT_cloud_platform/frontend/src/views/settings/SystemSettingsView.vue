<template>
  <div class="page-shell page-padding">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">系统设置</p>
        <h1>连接 & 主题</h1>
        <p class="lede">快速调整 API、MQTT 与主题偏好。</p>

        <div class="app-hero__badges">
          <el-tag effect="dark" type="info">API {{ form.apiBase || '/api' }}</el-tag>
          <el-tag effect="plain" type="success">MQTT {{ form.mqttWsUrl || 'ws://localhost:8083/mqtt' }}</el-tag>
          <el-tag effect="plain" :type="form.darkMode ? 'warning' : 'info'">主题 {{ form.darkMode ? '暗色' : '亮色' }}</el-tag>
        </div>

        <div class="app-hero__actions">
          <el-button type="primary" @click="save">保存</el-button>
          <el-button plain @click="reset">恢复默认</el-button>
          <el-button text @click="exportConfig">导出配置</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><Link /></el-icon></div>
            <div class="mini-meta">
              <p>API</p>
              <strong>REST</strong>
              <span>基础地址</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon success"><el-icon><Connection /></el-icon></div>
            <div class="mini-meta">
              <p>MQTT</p>
              <strong>WebSocket</strong>
              <span>实时数据</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><Moon /></el-icon></div>
            <div class="mini-meta">
              <p>主题</p>
              <strong>{{ form.darkMode ? '暗色' : '亮色' }}</strong>
              <span>界面偏好</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-card shadow="never">
      <template #header><span>连接配置</span></template>
      <el-form label-width="160px" :model="form" :inline="false" style="max-width: 720px;">
        <el-form-item label="API 基础地址">
          <el-input v-model="form.apiBase" placeholder="如 http://localhost:8080/api" />
          <p class="muted">用于所有 REST 调用，留空使用默认 /api。</p>
        </el-form-item>
        <el-form-item label="MQTT WebSocket 地址">
          <el-input v-model="form.mqttWsUrl" placeholder="如 ws://localhost:8083/mqtt" />
          <p class="muted">用于仪表盘实时连接，保存后需要重新连接。</p>
        </el-form-item>
        <el-form-item label="界面主题">
          <el-switch
            v-model="form.darkMode"
            inline-prompt
            active-text="暗色"
            inactive-text="亮色"
          />
        </el-form-item>
        <el-form-item label="操作">
          <el-button @click="exportConfig">导出配置</el-button>
          <el-button @click="reset">恢复默认</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { reactive } from 'vue';
import { ElMessage } from 'element-plus';
import { Connection, Link, Moon } from '@element-plus/icons-vue';
import { useAppStore } from '@/stores/app';
import api from '@/api/http';

const appStore = useAppStore();

const form = reactive({
  apiBase: appStore.apiBase,
  mqttWsUrl: appStore.mqttWsUrl,
  darkMode: appStore.theme === 'dark'
});

function save() {
  appStore.setApiBase(form.apiBase || '/api');
  api.defaults.baseURL = form.apiBase || '/api';
  appStore.setMqttWsUrl(form.mqttWsUrl || 'ws://localhost:8083/mqtt');
  appStore.setTheme(form.darkMode ? 'dark' : 'light');
  ElMessage.success('设置已保存');
}

function reset() {
  form.apiBase = import.meta.env.VITE_API_BASE_URL || '/api';
  form.mqttWsUrl = import.meta.env.VITE_MQTT_WS_URL || 'ws://localhost:8083/mqtt';
  form.darkMode = false;
}

function exportConfig() {
  const blob = new Blob([JSON.stringify(form, null, 2)], { type: 'application/json' });
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = 'iot_web_config.json';
  link.click();
  URL.revokeObjectURL(link.href);
}
</script>
