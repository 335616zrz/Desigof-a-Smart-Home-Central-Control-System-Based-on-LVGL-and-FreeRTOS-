<template>
  <main class="fusion-stage" aria-hidden="true">
    <div class="glow glow-1"></div>
    <div class="glow glow-2"></div>
    <div class="glow glow-3"></div>

    <div class="glass glass-1"></div>
    <div class="glass glass-2"></div>
    <div class="glass glass-3"></div>

    <div class="soft soft-1"></div>
    <div class="soft soft-2"></div>

    <div class="bubble bubble-1"></div>
    <div class="bubble bubble-2"></div>
    <div class="bubble bubble-3"></div>
  </main>
</template>

<style scoped>
.fusion-stage {
  --fusion-sky: #e9f3ff;
  --fusion-mint: #c7f6ed;
  --fusion-lilac: #e6ddff;
  --fusion-peach: #ffe9d6;
  --fusion-rose: #ffe3f1;
  --fusion-ink: rgba(12, 26, 64, 0.12);
  --glass-fill: rgba(255, 255, 255, 0.6);
  --glass-stroke: rgba(255, 255, 255, 0.38);

  position: relative;
  min-height: 100vh;
  overflow: hidden;
  background:
    radial-gradient(120% 95% at 12% 18%, rgba(199, 246, 237, 0.8), transparent 55%),
    radial-gradient(110% 82% at 82% -4%, rgba(230, 221, 255, 0.75), transparent 52%),
    radial-gradient(120% 120% at 84% 86%, rgba(255, 233, 214, 0.85), transparent 60%),
    radial-gradient(140% 120% at 10% 90%, rgba(255, 227, 241, 0.6), transparent 62%),
    linear-gradient(135deg, #fdfdff 0%, #eef3ff 42%, #f8fbff 100%);
  display: grid;
  place-items: center;
  padding: clamp(16px, 4vw, 48px);
  isolation: isolate;
}

/* 暗夜模式下使用更深的底色与柔和高光，避免整页过亮 */
body.dark .fusion-stage {
  --fusion-sky: #0f172a;
  --fusion-mint: rgba(56, 189, 248, 0.35);
  --fusion-lilac: rgba(129, 140, 248, 0.32);
  --fusion-peach: rgba(250, 204, 21, 0.28);
  --fusion-rose: rgba(244, 114, 182, 0.28);
  --fusion-ink: rgba(0, 0, 0, 0.45);
  --glass-fill: rgba(30, 41, 59, 0.55);
  --glass-stroke: rgba(148, 163, 184, 0.22);

  background:
    radial-gradient(120% 95% at 12% 18%, rgba(56, 189, 248, 0.18), transparent 55%),
    radial-gradient(110% 82% at 82% -4%, rgba(99, 102, 241, 0.18), transparent 52%),
    radial-gradient(120% 120% at 84% 86%, rgba(250, 204, 21, 0.18), transparent 60%),
    radial-gradient(140% 120% at 10% 90%, rgba(236, 72, 153, 0.16), transparent 62%),
    linear-gradient(135deg, #0f1626 0%, #111c2f 42%, #0b1322 100%);
}

.fusion-stage::after {
  content: '';
  position: absolute;
  inset: 0;
  background: radial-gradient(circle at 50% 50%, rgba(255, 255, 255, 0.35), transparent 55%);
  mix-blend-mode: overlay;
  pointer-events: none;
}

body.dark .fusion-stage::after {
  background: radial-gradient(circle at 50% 50%, rgba(56, 189, 248, 0.12), transparent 55%);
  mix-blend-mode: normal;
  opacity: 0.5;
}

.glow {
  position: absolute;
  width: 48vmax;
  height: 48vmax;
  filter: blur(90px);
  opacity: 0.78;
  mix-blend-mode: screen;
  transform: translate3d(0, 0, 0);
  animation: drift 22s ease-in-out infinite;
}

.glow-1 {
  top: -10vmax;
  left: -8vmax;
  background: radial-gradient(circle at 40% 40%, var(--fusion-mint), transparent 55%);
}

.glow-2 {
  bottom: -12vmax;
  right: -10vmax;
  background: radial-gradient(circle at 60% 60%, var(--fusion-lilac), transparent 58%);
  animation-direction: reverse;
}

.glow-3 {
  top: 18%;
  right: 26%;
  width: 36vmax;
  height: 36vmax;
  background: radial-gradient(circle at 50% 50%, var(--fusion-rose), transparent 60%);
  filter: blur(110px);
  opacity: 0.72;
}

.glass {
  position: absolute;
  border-radius: 28px;
  background: linear-gradient(135deg, var(--glass-fill), rgba(255, 255, 255, 0.32));
  border: 1px solid var(--glass-stroke);
  box-shadow:
    0 28px 58px -30px rgba(12, 26, 64, 0.4),
    0 14px 32px -22px rgba(12, 26, 64, 0.25),
    inset 0 1px 0 rgba(255, 255, 255, 0.9),
    inset 0 -12px 28px rgba(255, 255, 255, 0.2);
  backdrop-filter: blur(22px) saturate(145%);
  -webkit-backdrop-filter: blur(22px) saturate(145%);
  transform: translate3d(0, 0, 0);
}

.glass::before {
  content: '';
  position: absolute;
  inset: 0;
  border-radius: inherit;
  background: linear-gradient(120deg, rgba(255, 255, 255, 0.38) 0%, rgba(255, 255, 255, 0.08) 38%, transparent 62%);
  mix-blend-mode: screen;
}

.glass::after {
  content: '';
  position: absolute;
  inset: 12% 14%;
  border-radius: inherit;
  background: radial-gradient(circle at 20% 20%, rgba(255, 255, 255, 0.4), transparent 50%);
  opacity: 0.85;
}

.glass-1 {
  width: clamp(260px, 30vw, 420px);
  aspect-ratio: 1 / 1.05;
  transform: translate3d(2%, -2%, 0) rotate(-6deg);
}

.glass-2 {
  width: clamp(200px, 24vw, 320px);
  aspect-ratio: 1 / 1;
  top: 18%;
  left: 20%;
  transform: translate3d(0, 0, 0) rotate(10deg);
}

.glass-3 {
  width: clamp(180px, 20vw, 280px);
  aspect-ratio: 1 / 1;
  bottom: 16%;
  right: 18%;
  transform: translate3d(0, 0, 0) rotate(-14deg);
}

.soft {
  position: absolute;
  border-radius: 999px;
  background: linear-gradient(145deg, rgba(255, 255, 255, 0.92), rgba(255, 255, 255, 0.68));
  box-shadow:
    -18px -18px 34px rgba(255, 255, 255, 0.86),
    18px 18px 32px rgba(165, 191, 255, 0.25),
    inset 1px 1px 0 rgba(255, 255, 255, 0.7);
  filter: drop-shadow(0 24px 28px rgba(13, 52, 84, 0.16));
}

.soft-1 {
  width: clamp(230px, 26vw, 360px);
  height: clamp(68px, 9vw, 120px);
  bottom: 12%;
  left: 8%;
}

.soft-2 {
  width: clamp(170px, 20vw, 280px);
  height: clamp(60px, 8vw, 108px);
  top: 14%;
  right: 10%;
  box-shadow:
    -14px -14px 28px rgba(255, 255, 255, 0.82),
    14px 14px 28px rgba(200, 220, 255, 0.3),
    inset 1px 1px 0 rgba(255, 255, 255, 0.68);
}

.bubble {
  position: absolute;
  border-radius: 50%;
  mix-blend-mode: screen;
  background:
    radial-gradient(80% 80% at 28% 28%, rgba(255, 255, 255, 0.9), rgba(255, 255, 255, 0) 65%),
    radial-gradient(60% 60% at 78% 78%, rgba(188, 222, 255, 0.9), rgba(188, 222, 255, 0) 70%),
    radial-gradient(85% 85% at 50% 50%, rgba(255, 233, 214, 0.75), rgba(255, 233, 214, 0) 70%);
  box-shadow: 0 18px 38px -20px rgba(13, 52, 84, 0.25);
  animation: float 16s ease-in-out infinite alternate;
}

.bubble-1 {
  width: clamp(140px, 16vw, 240px);
  top: 30%;
  left: 44%;
}

.bubble-2 {
  width: clamp(110px, 12vw, 190px);
  bottom: 22%;
  right: 32%;
  animation-delay: 3s;
}

.bubble-3 {
  width: clamp(90px, 10vw, 150px);
  top: 24%;
  left: 26%;
  animation-delay: 6s;
}

@keyframes float {
  0% {
    transform: translate3d(0, 0, 0) scale(1);
  }
  50% {
    transform: translate3d(6%, -4%, 0) scale(1.05);
  }
  100% {
    transform: translate3d(-6%, 6%, 0) scale(0.98);
  }
}

@keyframes drift {
  0% {
    transform: translate3d(0, 0, 0);
  }
  50% {
    transform: translate3d(4%, 3%, 0);
  }
  100% {
    transform: translate3d(-4%, -2%, 0);
  }
}

@media (max-width: 768px) {
  .fusion-stage {
    padding: 12px;
  }

  .glass {
    border-radius: 20px;
  }

  .glass-2 {
    top: 16%;
    left: 10%;
  }

  .glass-3 {
    right: 8%;
    bottom: 14%;
  }
}
</style>
