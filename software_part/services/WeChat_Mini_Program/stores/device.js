import {
  createDevice,
  deleteDevice,
  getDevice,
  listDevices,
  provisionDevice,
  updateDevice,
} from '../api/device.js'
import { defineStore } from './pinia.js'

function unwrap(resp) {
  return resp?.data ?? resp ?? null
}

export const useDeviceStore = defineStore('device', {
  state: () => ({
    items: [],
    total: 0,
    loading: false,
    current: null,
  }),
  actions: {
    async fetchList(params) {
      this.loading = true
      try {
        const resp = await listDevices(params)
        const data = unwrap(resp)
        this.items = data?.items || []
        this.total = data?.total || 0
        return data
      } finally {
        this.loading = false
      }
    },

    async fetchDetail(deviceId) {
      const resp = await getDevice(deviceId)
      const data = unwrap(resp)
      this.current = data
      return data
    },

    async create(payload) {
      const resp = await createDevice(payload)
      const data = unwrap(resp)
      return data
    },

    async update(deviceId, payload) {
      const resp = await updateDevice(deviceId, payload)
      const data = unwrap(resp)
      return data
    },

    async remove(deviceId) {
      await deleteDevice(deviceId)
      // Optimistic local update.
      this.items = (this.items || []).filter((d) => d?.deviceId !== deviceId)
      this.total = Math.max(0, (this.total || 0) - 1)
    },

    async provision(deviceId, payload) {
      const resp = await provisionDevice(deviceId, payload)
      // Provisioning endpoint returns raw JSON (not ApiResponse envelope).
      return resp
    },
  },
})

