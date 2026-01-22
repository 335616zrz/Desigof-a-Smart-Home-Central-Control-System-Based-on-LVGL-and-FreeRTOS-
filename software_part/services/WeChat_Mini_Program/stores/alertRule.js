import {
  createAlertRule,
  deleteAlertRule,
  getAlertRule,
  getCurrentSeason,
  listAlertRules,
  listSystemTemplates,
  listUserRules,
  toggleAlertRule,
  updateAlertRule,
} from '../api/alertRule.js'
import { defineStore } from './pinia.js'

export const useAlertRuleStore = defineStore('alertRule', {
  state: () => ({
    rules: [],
    userRules: [],
    templates: [],
    season: null,
    loading: false,
    current: null,
  }),
  actions: {
    async fetchRules(enabled) {
      this.loading = true
      try {
        const resp = await listAlertRules(enabled)
        this.rules = resp || []
        return this.rules
      } finally {
        this.loading = false
      }
    },

    async fetchUserRules(deviceId) {
      this.loading = true
      try {
        const resp = await listUserRules(deviceId)
        this.userRules = resp || []
        return this.userRules
      } finally {
        this.loading = false
      }
    },

    async fetchCurrentSeason() {
      this.loading = true
      try {
        const resp = await getCurrentSeason()
        this.season = resp || null
        return this.season
      } finally {
        this.loading = false
      }
    },

    async fetchTemplates(season) {
      this.loading = true
      try {
        const resp = await listSystemTemplates(season)
        this.templates = resp || []
        return this.templates
      } finally {
        this.loading = false
      }
    },

    async fetchDetail(id) {
      const resp = await getAlertRule(id)
      this.current = resp || null
      return this.current
    },

    async create(payload) {
      const resp = await createAlertRule(payload)
      return resp
    },

    async update(id, payload) {
      const resp = await updateAlertRule(id, payload)
      return resp
    },

    async toggle(id, enabled) {
      await toggleAlertRule(id, enabled)
      // Best-effort local update.
      const updateLocal = (arr) => {
        const idx = (arr || []).findIndex((r) => r?.id === id)
        if (idx >= 0) arr[idx].enabled = enabled
      }
      updateLocal(this.rules)
      updateLocal(this.userRules)
      updateLocal(this.templates)
    },

    async remove(id) {
      await deleteAlertRule(id)
      const filterOut = (arr) => (arr || []).filter((r) => r?.id !== id)
      this.rules = filterOut(this.rules)
      this.userRules = filterOut(this.userRules)
      this.templates = filterOut(this.templates)
    },
  },
})
