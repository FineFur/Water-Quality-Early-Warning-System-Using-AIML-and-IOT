import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  server: {
    host: '0.0.0.0',   // bind to all interfaces (fixes IPv4/IPv6 mismatch)
    port: 5173,
    proxy: {
      // Proxy /api calls to the FastAPI backend — eliminates any CORS issues
      '/api': {
        target: 'http://127.0.0.1:8000',
        changeOrigin: true,
      },
    },
  },
});