(() => {
  const form = document.getElementById('query-form');
  const rawEl = document.getElementById('raw');
  const notice = document.getElementById('notice');
  // 已去除“实况/预报”分段切换按钮
  const selProv = document.getElementById('sel-province');
  const selCity = document.getElementById('sel-city');
  const selCounty = document.getElementById('sel-county');

  const hero = {
    box: document.getElementById('hero'),
    city: document.getElementById('hero-city'),
    update: document.getElementById('hero-update'),
    temp: document.getElementById('hero-temp'),
    icon: document.getElementById('hero-icon'),
    weather: document.getElementById('hero-weather'),
    chips: document.getElementById('hero-chips'),
  };
  const forecastWrap = document.getElementById('forecast');
  const forecastCards = document.getElementById('forecast-cards');

  function showNotice(msg){
    notice.hidden = !msg;
    notice.textContent = msg || '';
  }

  const hour = new Date().getHours();
  const isNightNow = (hour < 6 || hour >= 18);

  function pickIcon(text){
    const t = (text || '').toString();
    if (/雨夹雪/.test(t)) return 'sleet';
    if (/冰雹/.test(t)) return 'hail';
    if (/雷阵雨/.test(t)) return 'thunder';
    if (/阵雨/.test(t)) return 'shower';
    if (/(小雨|细雨|毛毛雨)/.test(t)) return 'drizzle';
    if (/(中雨|大雨|暴雨|大到暴雨|大暴雨)/.test(t)) return 'heavyrain';
    if (/雷/.test(t)) return 'thunder';
    if (/(大雪|中雪|小雪|阵雪|暴雪)/.test(t)) return 'snow';
    if (/(沙尘暴|扬沙|浮尘)/.test(t)) return 'sand';
    if (/雾/.test(t)) return 'fog';
    if (/霾/.test(t)) return 'haze';
    if (/风/.test(t)) return 'wind';
    if (/阴/.test(t)) return 'overcast';
    if (/多云/.test(t)) return 'cloud';
    if (/晴/.test(t)) return isNightNow ? 'moon' : 'sun';
    if (/雨/.test(t)) return 'rain';
    return 'cloud';
  }

  function applyHeroTheme(_text){
    // 统一浅色：保持明亮背景，不再根据天气切换深色
    hero.box.style.background = 'var(--grad-day)';
    document.documentElement.setAttribute('data-theme', 'light');
  }

  function setIcon(el, name){
    el.innerHTML = `<use href="/assets/icons.svg#${name}"></use>`;
  }

  function renderLive(json){
    const live = (json && json.lives && json.lives[0]) || null;
    if(!live){ showNotice('没有实况数据'); return; }
    hero.city.textContent = `${live.province || ''} ${live.city || ''}`.trim() || '—';
    hero.update.textContent = live.reporttime || '—';
    hero.temp.textContent = (live.temperature ? Math.round(Number(live.temperature)) : '--');
    hero.weather.textContent = live.weather || '—';
    setIcon(hero.icon, pickIcon(live.weather));
    applyHeroTheme(live.weather);
    const chips = [];
    chips.push(`风向 ${live.winddirection || '-'}`);
    chips.push(`风力 ${live.windpower || '-'}`);
    chips.push(`湿度 ${live.humidity || '-'}%`);
    hero.chips.innerHTML = chips.map(t => `<span class="chip">${t}</span>`).join('');
  }

  function renderForecast(json){
    const fc = (json && json.forecasts && json.forecasts[0]) || null;
    if(!fc){ forecastWrap.style.display = 'none'; return; }
    forecastWrap.style.display = '';
    const items = (fc.casts || []).map(c => {
      const icon = pickIcon(c.dayweather || c.nightweather);
      return `<div class="fcard">
        <div class="day">${c.date || '-'} · 周${c.week || '-'}</div>
        <div class="t"><svg class="icon"><use href="/assets/icons.svg#${icon}"></use></svg> <span class="wx">${c.dayweather || '-'} / ${c.nightweather || '-'}</span></div>
        <div class="range">${c.nighttemp || '-'}° ~ ${c.daytemp || '-'}°</div>
        <div class="day">风 ${c.daywind || '-'} / ${c.nightwind || '-'} · 力 ${c.daypower || '-'} / ${c.nightpower || '-'}</div>
      </div>`;
    });
    forecastCards.innerHTML = items.join('');
  }

  async function fetchJson(url){
    const resp = await fetch(url);
    const text = await resp.text();
    rawEl.textContent = text;
    let json = null; try{ json = JSON.parse(text); }catch(_){ }
    if(!resp.ok){
      const msg = (json && (json.info || json.error)) || `请求失败（HTTP ${resp.status}）`;
      throw new Error(msg);
    }
    return json;
  }

  // --- Region helpers ---
  function fillSelect(sel, items, placeholder){
    const opts = [];
    if (placeholder) opts.push(`<option value="">${placeholder}</option>`);
    for(const it of items){ opts.push(`<option value="${it.code}">${it.name}</option>`); }
    sel.innerHTML = opts.join('');
    sel.disabled = items.length === 0;
  }

  async function loadProvinces(defaultCode){
    try{
      const data = await fetchJson('/api/regions/provinces');
      const list = data.provinces || [];
      fillSelect(selProv, list, '选择省份');
      if (defaultCode && list.find(x=>x.code===defaultCode)) selProv.value = defaultCode;
      await loadCities(selProv.value || (list[0] && list[0].code) || '');
    }catch(e){ /* ignore */ }
  }

  async function loadCities(provCode, defaultCity){
    if (!provCode){ fillSelect(selCity, [], '选择城市'); fillSelect(selCounty, [], '选择区县'); return; }
    const data = await fetchJson(`/api/regions/cities?province=${encodeURIComponent(provCode)}`);
    const items = data.cities || [];
    fillSelect(selCity, items, '选择城市');
    if (defaultCity && items.find(x=>x.code===defaultCity)) selCity.value = defaultCity;
    await loadCounties(selCity.value || (items[0] && items[0].code) || '', false);
  }

  async function loadCounties(cityCode, autoQuery=true){
    if (!cityCode){ fillSelect(selCounty, [], '选择区县'); return; }
    const data = await fetchJson(`/api/regions/counties?city=${encodeURIComponent(cityCode)}`);
    const items = data.counties || [];
    fillSelect(selCounty, items, '选择区县');
    if (items.length){ selCounty.value = items[0].code; if (autoQuery) doQuery(selCounty.value); }
    else { if (autoQuery) doQuery(cityCode); }
  }

  async function doQuery(city){
    showNotice('');
    const btn = form.querySelector('button.primary');
    btn.disabled = true;
    try{
      const live = await fetchJson(`/api/weather?city=${encodeURIComponent(city)}&extensions=base`);
      renderLive(live);
      try{
        const fc = await fetchJson(`/api/weather?city=${encodeURIComponent(city)}&extensions=all`);
        renderForecast(fc);
      }catch(_){ /* ignore */ }
    }catch(err){
      showNotice(err.message || String(err));
    }finally{
      btn.disabled = false;
    }
  }

  // 已移除分段切换逻辑

  form.addEventListener('submit', (e) => {
    e.preventDefault();
    const code = selCounty.value || selCity.value || selProv.value;
    if(!code) { showNotice('请选择省/市/区县'); return; }
    doQuery(code);
  });

  selProv && selProv.addEventListener('change', () => {
    loadCities(selProv.value).catch(()=>{});
  });
  selCity && selCity.addEventListener('change', () => {
    loadCounties(selCity.value).catch(()=>{});
  });
  selCounty && selCounty.addEventListener('change', () => {
    if (selCounty.value) doQuery(selCounty.value);
  });

  // 初始示例
  loadProvinces('110000').then(()=>{/* autoQuery occurs after counties loaded */}).catch(()=>{});
  // 保底：若地区接口失败，仍按东城区查询
  setTimeout(()=>{ if (!hero.city.textContent || hero.city.textContent === '—') doQuery('110101'); }, 1500);
  // 注册 Service Worker 以支持 PWA 安装/离线
  if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
      navigator.serviceWorker.register('/sw.js').catch(() => {});
    });
  }
})();
