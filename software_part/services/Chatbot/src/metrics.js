class MetricsRegistry {
  constructor() {
    this.counters = new Map();
    this.histograms = new Map();
  }

  increment(name, value = 1) {
    const current = this.counters.get(name) || 0;
    this.counters.set(name, current + value);
  }

  observe(name, value) {
    if (!Number.isFinite(value)) return;
    const current = this.histograms.get(name) || {
      count: 0,
      sum: 0,
      min: Number.POSITIVE_INFINITY,
      max: Number.NEGATIVE_INFINITY,
    };
    current.count += 1;
    current.sum += value;
    if (value < current.min) current.min = value;
    if (value > current.max) current.max = value;
    this.histograms.set(name, current);
  }

  snapshot() {
    const counters = {};
    for (const [key, value] of this.counters.entries()) {
      counters[key] = value;
    }
    const histograms = {};
    for (const [key, value] of this.histograms.entries()) {
      histograms[key] = {
        count: value.count,
        sum: value.sum,
        min: value.count ? value.min : 0,
        max: value.count ? value.max : 0,
        avg: value.count ? value.sum / value.count : 0,
      };
    }
    return { counters, histograms };
  }
}

const registry = new MetricsRegistry();

module.exports = {
  metrics: registry,
};
