<template>
  <div class="login-page-wrapper">
    <div class="scene">
      <div class="theme-switch" aria-label="主题切换">
        <button
          v-for="mode in themeModes"
          :key="mode"
          :data-theme="mode"
          :class="{ active: themeMode === mode }"
          :aria-pressed="themeMode === mode"
          @click="setTheme(mode)"
        >
          {{ themeLabels[mode] }}
        </button>
      </div>

      <div class="layout">
        <div :class="['panel', 'panel-left', { shake }]"><!-- left card -->
          <div class="logo-row">
            <div class="logo-icon" aria-hidden="true">
              <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                <path
                  d="M7.5 10V7.5C7.5 4.46 9.96 2 13 2C16.04 2 18.5 4.46 18.5 7.5V10"
                  stroke="currentColor"
                  stroke-width="1.6"
                  stroke-linecap="round"
                />
                <rect x="4" y="9.5" width="18" height="12" rx="2.4" stroke="currentColor" stroke-width="1.6" />
                <circle cx="13" cy="15.5" r="1.6" fill="currentColor" />
              </svg>
            </div>
            <div class="logo-text">
              <div class="brand">物联网云平台</div>
              <div class="sub">安全的设备运营中台</div>
            </div>
          </div>

          <h1>欢迎回来</h1>
          <p class="desc">登录以管理您的设备。</p>

          <form class="login-form" @submit.prevent>
            <label class="field">
              <span class="label"><span class="required">*</span> 用户名（&gt;=3 个字符）</span>
              <input
                type="text"
                name="username"
                placeholder="请输入用户名"
                autocomplete="username"
                v-model.trim="form.username"
                @input="handleInput"
              />
              <div class="helper">{{ helpers.username }}</div>
            </label>

            <label class="field">
              <span class="label"><span class="required">*</span> 密码（&gt;=6 位）</span>
              <div class="input-wrap">
                <input
                  :type="showPassword ? 'text' : 'password'"
                  name="password"
                  placeholder="请输入密码"
                  autocomplete="current-password"
                  v-model.trim="form.password"
                  @input="handleInput"
                />
                <span class="eye" aria-hidden="true" @click="showPassword = !showPassword" :title="showPassword ? '隐藏密码' : '显示密码'">
                  <svg v-if="!showPassword" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path
                      d="M2.5 12C4.3 7.7 8.3 5 12 5C15.7 5 19.7 7.7 21.5 12C19.7 16.3 15.7 19 12 19C8.3 19 4.3 16.3 2.5 12Z"
                      stroke="currentColor"
                      stroke-width="1.4"
                      stroke-linecap="round"
                      stroke-linejoin="round"
                    />
                    <circle cx="12" cy="12" r="3" stroke="currentColor" stroke-width="1.4" />
                  </svg>
                  <svg v-else viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M3 3L21 21" stroke="currentColor" stroke-width="1.4" stroke-linecap="round"/>
                    <path
                      d="M10.5 10.5C10.18 10.82 10 11.28 10 11.8C10 12.9 10.9 13.8 12 13.8C12.52 13.8 12.98 13.62 13.3 13.3M19.5 16.2C17.8 17.5 15.8 18.3 13.5 18.3C9.8 18.3 6.3 16.1 4 12.8C5.2 10.8 6.8 9.2 8.7 8.2M14.8 7.1C15.5 7.3 16.2 7.6 16.9 8C19.2 9.4 21.2 11.5 22.5 14C21.8 15.3 20.9 16.4 19.9 17.3M12 5.5C12.3 5.5 12.7 5.5 13 5.6"
                      stroke="currentColor"
                      stroke-width="1.4"
                      stroke-linecap="round"
                      stroke-linejoin="round"
                    />
                  </svg>
                </span>
              </div>
              <div class="helper">{{ helpers.password }}</div>
            </label>

            <div class="action-row">
              <button
                type="button"
                :disabled="loading"
                :class="['primary-btn', { loading, 'pulse-success': pulseSuccess } ]"
                @click="handleLogin"
              >
                {{ loading ? '登录中…' : '登录' }}
              </button>
              <div :class="['inline-result', inlineState.type]" id="inlineResult" aria-live="polite">
                <span class="icon">{{ inlineState.icon }}</span>
                <span class="text">{{ inlineState.text }}</span>
              </div>
            </div>
          </form>

          <div class="auth-footer">
            <span>没有账号？</span>
            <RouterLink to="/register">去注册</RouterLink>
          </div>
        </div>

        <div class="panel panel-right">
          <div class="hero">
            <h2>实时可见，可靠可控。</h2>
            <p>统一管理设备、监测指标并处理告警。</p>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, reactive, ref } from 'vue';
import { useRouter } from 'vue-router';
import { useAppStore } from '@/stores/app';
import { useAuthStore } from '@/stores/auth';

const router = useRouter();
const auth = useAuthStore();
const app = useAppStore();

const form = reactive({ username: '', password: '' });
const helpers = reactive({ username: '', password: '' });
const inlineState = reactive({ type: '', icon: '', text: '' });

const loading = ref(false);
const pulseSuccess = ref(false);
const shake = ref(false);
const attempts = ref(0);
const showPassword = ref(false);

const themeModes = ['light', 'auto', 'dark'] as const;
const themeLabels = { light: '浅色', auto: '自动', dark: '暗夜' };

const themeMode = computed(() => app.theme);
const setTheme = (mode: 'light' | 'auto' | 'dark') => app.setTheme(mode);

const setInline = (success, text) => {
  inlineState.type = success ? 'success' : 'error';
  inlineState.icon = success ? '✔' : '✕';
  inlineState.text = text;
};

const updateHelpers = () => {
  const u = form.username.trim();
  const p = form.password.trim();
  helpers.username = u.length === 0 ? '请输入用户名' : u.length < 3 ? '至少 3 个字符' : '';
  helpers.password = p.length === 0 ? '请输入密码' : p.length < 6 ? '至少 6 位' : '';
};

let helperTimerId;
const handleInput = () => {
  if (helperTimerId) window.clearTimeout(helperTimerId);
  helperTimerId = window.setTimeout(updateHelpers, 260);
};

let shakeTimerId;
const triggerShake = () => {
  shake.value = true;
  if (shakeTimerId) window.clearTimeout(shakeTimerId);
  shakeTimerId = window.setTimeout(() => { shake.value = false; }, 400);
};

let pulseTimerId;
const handleSuccess = () => {
  attempts.value = 0;
  setInline(true, '登录成功，正在进入');
  pulseSuccess.value = true;
  if (pulseTimerId) window.clearTimeout(pulseTimerId);
  pulseTimerId = window.setTimeout(() => { pulseSuccess.value = false; }, 450);
};

const handleFail = () => {
  attempts.value += 1;
  let msg = '请检查用户名或密码';
  if (attempts.value === 2) msg = '提示：用户名≥3字符，密码≥6位；忘记密码可重置';
  if (attempts.value >= 3) msg = '多次失败，请检查账号或联系支持';
  setInline(false, msg);
  triggerShake();
};

let loginTimerId;
const handleLogin = async () => {
  if (loading.value) return;
  updateHelpers();

  // Validate inputs
  const uOk = form.username.trim().length >= 3;
  const pOk = form.password.trim().length >= 6;

  if (!uOk || !pOk) {
    handleFail();
    return;
  }

  loading.value = true;
  setInline(false, '校验中…');

  try {
    // Call actual login API
    await auth.login(form.username, form.password);
    handleSuccess();

    // Navigate to redirect URL or dashboard after a short delay
    setTimeout(() => {
      const redirect = router.currentRoute.value.query.redirect as string || '/';
      router.push(redirect);
    }, 800);
  } catch (error: any) {
    attempts.value += 1;
    let msg = error?.response?.data?.message || '登录失败，请检查用户名或密码';
    if (attempts.value === 2) msg = '提示：用户名≥3字符，密码≥6位';
    if (attempts.value >= 3) msg = '多次失败，请检查账号或联系支持';
    setInline(false, msg);
    triggerShake();
    loading.value = false;
  }
};

onBeforeUnmount(() => {
  [helperTimerId, shakeTimerId, pulseTimerId, loginTimerId].forEach((id) => {
    if (id) window.clearTimeout(id);
  });
});
</script>

<style>
/* LoginView 全局CSS变量 - 必须不带scoped以便:root生效 */
:root {
  --bg-1: #f7f9fc;
  --bg-2: #eef2ff;
  --bg-3: #f4f7ff;
  --navy-border: rgba(120, 146, 180, 0.22);
  --text-main: #1f2d3d;
  --text-sub: #5c6b80;
  --panel-left: linear-gradient(180deg, rgba(255,255,255,0.92), rgba(245,248,255,0.94));
  --panel-right: linear-gradient(135deg, #35c4ff, #7ad4ff 55%, #a3e2ff);
  --panel-right-text: #0c3058;
  --input-bg: rgba(255,255,255,0.9);
  --input-border: rgba(120,146,180,0.22);
  --placeholder: #94a3b8;
  --inline-bg: rgba(255,255,255,0.94);
  --inline-border: rgba(120,146,180,0.22);
  --inline-success: #138f5a;
  --inline-success-border: rgba(19,143,90,0.25);
  --inline-error: #c44f30;
  --inline-error-border: rgba(196,79,48,0.2);
}

.theme-dark {
  --bg-1: #0f1626;
  --bg-2: #111c2f;
  --bg-3: #0b1322;
  --navy-border: rgba(255,255,255,0.16);
  --text-main: #e8f0ff;
  --text-sub: #b5c4dd;
  --panel-left: linear-gradient(180deg, rgba(22,30,48,0.94), rgba(16,24,40,0.96));
  --panel-right: linear-gradient(135deg, #0d7bd1, #0c5ca4 55%, #0b3f7e);
  --panel-right-text: #dfeeff;
  --input-bg: rgba(14, 22, 36, 0.92);
  --input-border: rgba(255,255,255,0.22);
  --placeholder: #c3d2ec;
  --inline-bg: rgba(18,28,46,0.82);
  --inline-border: rgba(255,255,255,0.16);
  --inline-success: #6ee7b7;
  --inline-success-border: rgba(110,231,183,0.35);
  --inline-error: #f5a89a;
  --inline-error-border: rgba(245,168,154,0.32);
}
</style>

<style scoped>
/* LoginView 组件样式 - 使用scoped防止污染其他组件 */
.login-page-wrapper {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  width: 100%;
  height: 100%;
  margin: 0;
  padding: 0;
  background: radial-gradient(circle at 20% 20%, rgba(255,255,255,0.8), transparent 40%),
              radial-gradient(circle at 80% 0%, rgba(200,220,255,0.6), transparent 35%),
              linear-gradient(135deg, var(--bg-1), var(--bg-2) 50%, var(--bg-3));
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
  perspective: 1600px;
  font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
  z-index: 1000;
}

* {
  box-sizing: border-box;
}

.scene {
  position: relative;
  width: min(1100px, 94vw);
  min-height: min(700px, 72vh);
  transform-style: flat;
  animation: none;
  filter: none;
  display: flex;
  align-items: center;
  justify-content: center;
}

@media (prefers-reduced-motion: reduce) {
  .scene { animation: none; }
  .halo, .blob, .sheen, .glow, .glass-pill, .info-progress.run { animation: none; }
}

@media (max-width: 1400px) {
  .layout { gap: clamp(14px, 2vw, 22px); padding: clamp(16px, 3vw, 32px); }
  .panel { flex: 1 1 420px; max-width: 500px; }
  .panel-right { padding-top: clamp(24px, 4vw, 36px); }
  .theme-switch { top: clamp(-36px, 2vw, -10px); right: clamp(18px, 4vw, 36px); }
}

.layout {
  position: relative;
  width: 100%;
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  gap: clamp(18px, 3vw, 32px);
  padding: clamp(18px, 4vw, 44px);
  z-index: 6;
  align-items: stretch;
}

.theme-switch {
  position: absolute;
  top: clamp(-38px, 2vw, -12px);
  right: clamp(18px, 4vw, 40px);
  display: flex;
  gap: 6px;
  background: rgba(255,255,255,0.9);
  border: 1px solid rgba(120,146,180,0.22);
  border-radius: 999px;
  padding: 6px;
  box-shadow: 0 8px 18px rgba(40,70,130,0.12);
  z-index: 10;
}

.theme-switch button {
  border: none;
  background: transparent;
  padding: 6px 12px;
  border-radius: 999px;
  font-size: 13px;
  color: var(--text-sub);
  cursor: pointer;
  transition: background 0.15s ease, color 0.15s ease;
}

.theme-switch button.active {
  background: linear-gradient(135deg, #0aa8ff, #0b77d6);
  color: #fff;
  box-shadow: 0 6px 12px rgba(10,168,255,0.25);
  min-width: 52px;
  text-align: center;
}

.theme-dark .theme-switch {
  background: rgba(12,24,44,0.9);
  border-color: rgba(255,255,255,0.12);
  box-shadow: 0 8px 18px rgba(0,0,0,0.35);
}

.theme-dark .theme-switch button { color: #c5d6f3; }
.theme-dark .theme-switch button.active { box-shadow: 0 6px 12px rgba(10,168,255,0.4); }

.panel {
  position: relative;
  flex: 1 1 460px;
  max-width: 540px;
  min-height: 600px;
  border-radius: 32px;
  overflow: hidden;
  border: 1px solid var(--navy-border);
  box-shadow: 0 12px 32px rgba(120, 160, 220, 0.18);
  backdrop-filter: none;
  -webkit-backdrop-filter: none;
}

.panel-left {
  max-width: 500px;
  background: var(--panel-left);
  color: var(--text-main);
  padding: clamp(16px, 4vw, 38px) clamp(16px, 4vw, 38px) clamp(24px, 4vw, 40px);
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.logo-row {
  display: flex;
  align-items: center;
  gap: 14px;
  margin-bottom: 2px;
}

.logo-icon {
  width: 46px;
  height: 46px;
  border-radius: 14px;
  display: grid;
  place-items: center;
  background: linear-gradient(135deg, rgba(10,168,255,0.12), rgba(255,255,255,0.6));
  color: #0a80d9;
  border: 1px solid rgba(10,168,255,0.2);
  box-shadow: 0 10px 18px rgba(10,168,255,0.18);
}

.theme-dark .logo-icon {
  background: linear-gradient(135deg, rgba(60,110,180,0.25), rgba(20,32,52,0.6));
  color: #7bc3ff;
  border: 1px solid rgba(123,195,255,0.35);
  box-shadow: 0 10px 20px rgba(0,0,0,0.35);
}

.logo-icon svg { width: 26px; height: 26px; }

.logo-text { display: flex; flex-direction: column; gap: 4px; }
.brand { font-weight: 700; letter-spacing: 0.5px; }
.sub { font-size: 13px; color: var(--text-sub); }

h1 {
  margin: 10px 0 0;
  font-size: clamp(26px, 3vw, 32px);
  letter-spacing: 0.4px;
}

.desc {
  margin: 4px 0 18px;
  color: var(--text-sub);
}

.login-form {
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.action-row {
  display: flex;
  align-items: center;
  gap: 12px;
}

.field { display: flex; flex-direction: column; gap: 8px; }
.label { font-size: 14px; color: var(--text-sub); }
.required { color: #ff6b6b; margin-right: 4px; }

.helper {
  min-height: 18px;
  font-size: 12px;
  color: #0a80d9;
  opacity: 0.9;
}

.input-wrap { position: relative; }

input[type="text"], input[type="password"] {
  width: 100%;
  padding: 12px 44px 12px 14px;
  border-radius: 10px;
  border: 1px solid var(--input-border);
  background: var(--input-bg);
  color: var(--text-main);
  font-size: 15px;
  transition: border 0.2s ease, box-shadow 0.2s ease, background 0.2s ease;
  box-shadow: inset 0 1px 2px rgba(0,0,0,0.12);
}

input::placeholder { color: var(--placeholder); }

input:focus {
  outline: none;
  border-color: var(--input-border);
  box-shadow: 0 0 0 2px rgba(0,0,0,0.06);
  background: var(--input-bg);
}

.eye {
  position: absolute;
  right: 12px;
  top: 50%;
  transform: translateY(-50%);
  color: #8aa3c4;
  cursor: pointer;
  transition: color 0.2s ease;
}

.eye:hover {
  color: #0aa8ff;
}

.eye svg { width: 20px; height: 20px; }

.primary-btn {
  margin-top: 4px;
  width: 100%;
  border: none;
  border-radius: 10px;
  padding: 12px;
  font-size: 15px;
  font-weight: 700;
  letter-spacing: 0.3px;
  color: #fff;
  background: linear-gradient(135deg, #0aa8ff, #0b77d6);
  box-shadow: 0 12px 25px rgba(6, 132, 230, 0.35);
  cursor: pointer;
  transition: transform 0.15s ease, box-shadow 0.15s ease;
}

.primary-btn:hover { transform: translateY(-1px); box-shadow: 0 14px 30px rgba(10,168,255,0.38); }
.primary-btn:active { transform: translateY(0); box-shadow: 0 8px 18px rgba(10,168,255,0.28); }
.primary-btn:disabled { opacity: 0.7; cursor: not-allowed; }

.primary-btn.loading {
  position: relative;
  color: transparent;
}

.primary-btn.loading::after {
  content: "";
  position: absolute;
  inset: 0;
  border-radius: inherit;
  border: 2px solid rgba(255,255,255,0.25);
  border-top-color: #fff;
  animation: spin 0.8s linear infinite;
}

.primary-btn.pulse-success {
  animation: pulseBtn 0.4s ease;
  background: linear-gradient(135deg, #19d18f, #0cbf7a);
  box-shadow: 0 12px 30px rgba(25, 209, 143, 0.35);
}

.primary-btn.pulse-success::before {
  content: "";
  position: absolute;
  left: 12px;
  right: 12px;
  bottom: 6px;
  height: 3px;
  border-radius: 6px;
  background: linear-gradient(90deg, rgba(255,255,255,0.85), rgba(255,255,255,0.3));
  animation: bar 0.35s ease;
}

.auth-footer {
  margin-top: auto;
  display: flex;
  gap: 8px;
  align-items: center;
  color: var(--text-sub);
  font-size: 14px;
}

.auth-footer a { color: #73c4ff; text-decoration: none; font-weight: 600; }
.auth-footer a:hover { text-decoration: underline; }

.panel-right {
  background: var(--panel-right);
  color: var(--panel-right-text);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: clamp(28px, 5vw, 52px) clamp(24px, 5vw, 48px) clamp(24px, 5vw, 48px);
  position: relative;
  overflow: hidden;
}

.panel-right::before { display: none; }
.panel-right::after  { display: none; }

.hero {
  position: relative;
  z-index: 2;
  max-width: 520px;
  text-align: center;
  margin: 0;
}

.hero h2 {
  margin: 0 0 14px;
  font-size: clamp(26px, 3vw, 36px);
  letter-spacing: 0.6px;
  font-weight: 800;
  color: var(--panel-right-text);
}

.hero p {
  margin: 0;
  color: var(--panel-right-text);
  font-size: 16px;
}

.inline-result {
  min-width: 180px;
  min-height: 44px;
  padding: 10px 12px;
  border-radius: 12px;
  background: var(--inline-bg);
  border: 1px solid var(--inline-border);
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
  color: var(--text-sub);
  box-shadow: 0 12px 24px rgba(40,70,130,0.08);
}

.inline-result .icon { font-weight: 800; opacity: 0.9; }
.inline-result.success { color: var(--inline-success); border-color: var(--inline-success-border); box-shadow: 0 12px 24px rgba(40,70,130,0.08); }
.inline-result.error { color: var(--inline-error); border-color: var(--inline-error-border); box-shadow: 0 12px 24px rgba(40,70,130,0.08); }

.shake { animation: shake 0.35s ease; }

@keyframes spin { to { transform: rotate(360deg); } }

@keyframes bar {
  from { transform: scaleX(0); transform-origin: left; opacity: 0.3; }
  to   { transform: scaleX(1); transform-origin: left; opacity: 1; }
}

/* Decorative layers removed for性能 */

@media (max-width: 900px) {
  .scene { height: 80vh; }
  .layout { flex-direction: column; gap: 20px; padding: 18px; }
  .panel { border-radius: 24px; }
  .panel-right { min-height: 260px; align-items: center; }
  .hero { margin-bottom: 12px; text-align: left; }
}
</style>
