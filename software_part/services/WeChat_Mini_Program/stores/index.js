import { createPinia } from './pinia.js'

let _pinia = null

export function setupStore(app) {
  _pinia = createPinia()
  app.use(_pinia)
  return _pinia
}

export function getPinia() {
  return _pinia
}

