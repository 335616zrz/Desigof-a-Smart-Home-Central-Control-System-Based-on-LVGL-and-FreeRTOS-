import { defineStore } from 'pinia';
import {
  createAlertRule,
  deleteAlertRule,
  getAlertRule,
  getCurrentSeason,
  listAlertRules,
  listSystemTemplates,
  listUserRules,
  toggleAlertRule,
  updateAlertRule
} from '@/api/alertRule';
import type { AlertRule, SeasonResponse } from '@/types/models';

interface AlertRuleState {
  rules: AlertRule[];
  systemTemplates: AlertRule[];
  userRules: AlertRule[];
  currentSeason: SeasonResponse | null;
  loading: boolean;
  current?: AlertRule;
}

export const useAlertRuleStore = defineStore('alertRule', {
  state: (): AlertRuleState => ({
    rules: [],
    systemTemplates: [],
    userRules: [],
    currentSeason: null,
    loading: false,
    current: undefined
  }),
  actions: {
    async fetchRules(enabled?: boolean) {
      this.loading = true;
      try {
        this.rules = await listAlertRules(enabled);
        this.systemTemplates = this.rules.filter(r => r.isSystemTemplate);
        this.userRules = this.rules.filter(r => !r.isSystemTemplate);
        return this.rules;
      } finally {
        this.loading = false;
      }
    },
    async fetchCurrentSeason() {
      this.currentSeason = await getCurrentSeason();
      return this.currentSeason;
    },
    async fetchSystemTemplates(season?: string) {
      this.loading = true;
      try {
        this.systemTemplates = await listSystemTemplates(season);
        return this.systemTemplates;
      } finally {
        this.loading = false;
      }
    },
    async fetchUserRules(deviceId?: string) {
      this.loading = true;
      try {
        this.userRules = await listUserRules(deviceId);
        return this.userRules;
      } finally {
        this.loading = false;
      }
    },
    async fetchOne(id: number) {
      this.current = await getAlertRule(id);
      return this.current;
    },
    async create(payload: AlertRule) {
      const created = await createAlertRule(payload);
      this.rules.unshift(created);
      this.userRules.unshift(created);
      return created;
    },
    async update(id: number, payload: AlertRule) {
      const updated = await updateAlertRule(id, payload);
      this.rules = this.rules.map(rule => (rule.id === id ? updated : rule));
      this.userRules = this.userRules.map(rule => (rule.id === id ? updated : rule));
      this.systemTemplates = this.systemTemplates.map(rule => (rule.id === id ? updated : rule));
      if (this.current?.id === id) this.current = updated;
      return updated;
    },
    async toggle(id: number, enabled: boolean) {
      await toggleAlertRule(id, enabled);
      this.rules = this.rules.map(rule => (rule.id === id ? { ...rule, enabled } : rule));
      this.userRules = this.userRules.map(rule => (rule.id === id ? { ...rule, enabled } : rule));
      this.systemTemplates = this.systemTemplates.map(rule => (rule.id === id ? { ...rule, enabled } : rule));
    },
    async remove(id: number) {
      await deleteAlertRule(id);
      this.rules = this.rules.filter(rule => rule.id !== id);
      this.userRules = this.userRules.filter(rule => rule.id !== id);
      this.systemTemplates = this.systemTemplates.filter(rule => rule.id !== id);
      if (this.current?.id === id) this.current = undefined;
    }
  }
});
