import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    vue(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    }
  },
  define: {
    __APP_VERSION__: JSON.stringify(process.env.npm_package_version),
  },
  server: {
    proxy: {
      '/api': {
        target: 'https://192.168.128.116',
        //changeOrigin: true,
        secure: false,
        https: true,
      },
      '/ws/v1': {
        target: 'wss://192.168.128.116',
        //changeOrigin: true,
        secure: false,
        https: true,
        ws: true
      }
    },
  },
})
