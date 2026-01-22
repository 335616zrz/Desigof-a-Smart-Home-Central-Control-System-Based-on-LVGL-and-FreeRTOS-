import { computed, reactive } from 'vue'

let activePinia = null

export function setActivePinia(pinia) {
  activePinia = pinia
}

export function getActivePinia() {
  return activePinia
}

// Minimal Pinia-like implementation for this project (options stores).
// It is intentionally small: enough for our stores + actions + getters usage.
export function createPinia() {
  const pinia = {
    state: reactive({}),
    _s: new Map(),
    install(app) {
      setActivePinia(pinia)
      // Provide for potential future use; not strictly required right now.
      app.provide('pinia', pinia)
      app.config.globalProperties.$pinia = pinia
    },
  }
  return pinia
}

export function defineStore(id, options) {
  return function useStore() {
    const pinia = getActivePinia()
    if (!pinia) {
      throw new Error('Pinia is not active. Did you forget to call app.use(createPinia())?')
    }

    if (pinia._s.has(id)) return pinia._s.get(id)

    const rawState = typeof options?.state === 'function' ? options.state() : {}
    const state = reactive(rawState)

    const store = state
    store.$id = id

    // Actions (bound to store as "this").
    const actions = options?.actions || {}
    Object.keys(actions).forEach((key) => {
      const fn = actions[key]
      if (typeof fn === 'function') store[key] = fn.bind(store)
    })

    // Getters as computed properties on the store.
    const getters = options?.getters || {}
    Object.keys(getters).forEach((key) => {
      const getter = getters[key]
      if (typeof getter !== 'function') return
      Object.defineProperty(store, key, {
        enumerable: true,
        get() {
          // Use `this` for compatibility with Pinia-style getters.
          return computed(() => getter.call(store, store)).value
        },
      })
    })

    pinia._s.set(id, store)
    return store
  }
}

