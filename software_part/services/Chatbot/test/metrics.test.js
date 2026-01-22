const test = require('node:test');
const assert = require('node:assert');

const { metrics } = require('../src/metrics');

test('metrics counter increments and histogram observe', () => {
  metrics.increment('demo_counter_total');
  metrics.increment('demo_counter_total', 2);
  metrics.observe('demo_latency_ms', 100);
  metrics.observe('demo_latency_ms', 50);

  const snapshot = metrics.snapshot();
  assert.strictEqual(snapshot.counters.demo_counter_total, 3);
  assert.deepStrictEqual(snapshot.histograms.demo_latency_ms, {
    count: 2,
    sum: 150,
    min: 50,
    max: 100,
    avg: 75,
  });
});
