import { defineConfig, loadEnv } from 'vite';
import vue from '@vitejs/plugin-vue';
import AutoImport from 'unplugin-auto-import/vite';
import Components from 'unplugin-vue-components/vite';
import { ElementPlusResolver } from 'unplugin-vue-components/resolvers';
import path from 'path';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), 'VITE_');

  const proxyApi = env.VITE_PROXY_API || 'http://backend:8080';
  const proxyWs = env.VITE_PROXY_WS || 'http://backend:8080';
  const proxyMqtt = env.VITE_PROXY_MQTT || 'ws://emqx:8083';

  return {
  plugins: [
    vue(),
    AutoImport({
      imports: ['vue', 'vue-router'],
      resolvers: [ElementPlusResolver()],
      dts: 'src/auto-imports.d.ts'
    }),
    Components({
      resolvers: [ElementPlusResolver()],
      dts: 'src/components.d.ts'
    })
  ],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, 'src')
    }
  },
  server: {
    host: '0.0.0.0',
    port: Number(env.VITE_PORT) || 8088,
    proxy: {
      '/api': {
        target: proxyApi,
        changeOrigin: true
      },
      '/ws': {
        target: proxyWs,
        ws: true,
        changeOrigin: true
      },
      '/mqtt': {
        target: proxyMqtt,
        ws: true,
        changeOrigin: true
      }
    }
  },
  preview: {
    host: '0.0.0.0',
    port: Number(env.VITE_PORT) || 8088
  },
  build: {
    outDir: 'dist',
    chunkSizeWarningLimit: 900,
    rollupOptions: {
      output: {
        manualChunks: {
          vue: ['vue', 'vue-router', 'pinia'],
          element: ['element-plus', '@element-plus/icons-vue'],
          echarts: ['echarts'],
          lodash: ['lodash-es']
        }
      }
    }
  }
  ,
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: 'src/tests/setup.ts',
    exclude: ['**/node_modules/**', '**/dist/**', '**/tests/e2e/**'],
    coverage: {
      provider: 'v8',
      reportsDirectory: 'coverage'
    }
  }
  };
});
