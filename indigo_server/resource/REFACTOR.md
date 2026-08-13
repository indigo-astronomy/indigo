# WebGUI Refactoring Plan

## Current State

| File | Lines | Role |
|------|-------|------|
| `indigo.js` | 301 | Vue 2 root instance + WebSocket layer |
| `components.js` | 968 | 17 global Vue 2 components + dark-mode helpers |
| `indigo.css` | 145 | State classes, layout utilities |
| `ctrl.html` | 111 | Device property tree browser |
| `mng.html` | 152 | Driver/WiFi/network management |
| `imager.html` | 482 | Camera imaging control |
| `mount.html` | 741 | Mount control + Celestial sky map |
| `guider.html` | 363 | Guider calibration + drift/correction graphs |
| `script.html` | 185 | Script engine CRUD |

**Core problems:**
- jQuery and Vue both touch the same DOM — breaking reactivity and requiring `guiSetup()` to be called on every property update to re-apply classes
- All application state lives in one global `INDIGO` Vue instance
- Dark mode is toggled by iterating over every `div`, `input`, and `canvas` in the DOM via jQuery selectors
- Status visibility (`#SUCCESS`, `#FAILURE`, `#MESSAGE`) is managed by jQuery `.show()`/`.hide()` outside Vue
- Components call `changeProperty()` directly instead of emitting events to parents
- `indigo-query-db` reads D3 internal `__data__` attributes — fragile coupling to Celestial internals
- The guider graphs and Celestial map live in page-level global state (arrays, `config` object mutation)

---

## Architectural Decisions

### 1. Keep Vue 2 or migrate to Vue 3?

**Decision: migrate to Vue 3 (Options API, CDN, no build step).**

Reasons:
- Vue 3's Proxy-based reactivity eliminates the need for `Vue.set()` / `Vue.delete()` calls — direct assignment and `delete` become reactive
- Vue 3 CDN build is a single file; no toolchain change needed
- Options API surface is nearly identical; migration is mostly mechanical
- Vue 3 `createApp` enables multiple independent roots (useful for future isolation)

### 2. Keep jQuery or remove it?

**Decision: remove jQuery entirely.**

jQuery is only present to support Bootstrap 4 tooltips/dropdowns, and to work around Vue's inability to imperatively touch the DOM. Both reasons go away when we upgrade Bootstrap and properly use Vue reactivity. jQuery adds ~87 KB minified and makes the data-flow direction unpredictable.

### 3. Keep Bootstrap 4 or upgrade to Bootstrap 5?

**Decision: upgrade to Bootstrap 5.**

Bootstrap 5 drops its jQuery dependency, renames utility classes to use logical properties (`ml-*` → `ms-*`, `mr-*` → `me-*`), and replaces `data-toggle`/`data-target` with `data-bs-toggle`/`data-bs-target`. This is the mechanical cost that buys us a jQuery-free baseline.

### 4. Keep `celestial.min.js` or rewrite the sky map?

**Decision: keep `celestial.min.js` but encapsulate it in a proper Vue component.**

Reasons to keep:
- `celestial.min.js` provides correct stereographic projection, coordinate transforms, and constellation/DSO/planet rendering using pre-existing data files (`stars.json`, `dsos.json`, etc.)
- Reimplementing a correct stereographic projection from scratch is a large, risk-prone effort
- The library itself works fine — the problem is how the current code integrates with it (global mutation of `config`, flag variables like `celestialVisible`, jQuery-based scrolling)

Reasons against a full rewrite:
- `d3-celestial` is unmaintained but stable for its purpose
- A canvas-only custom sky chart that handles the same feature set (star limits, DSO symbols, planet glyphs, custom markers, zoom, zenith-centered rotation) would be several hundred lines of trigonometry

What the encapsulation fixes:
- Move all Celestial state (`config`, `celestialVisible`, `currentCoordinates`, `targetCoordinates`, `objectCoordinates`) into a Vue component's `data()`
- Initialize Celestial in `mounted()`, tear down in `beforeUnmount()`
- Replace `$(map).scrollLeft()` / `.scrollTop()` with `this.$refs.map.scrollLeft`
- Replace canvas `mousedown` attachment with a Vue `@mousedown` handler on the component root

The one remaining pain point after encapsulation is the catalog search in `indigo-query-db`, which reads Celestial's D3 `__data__` attributes. This is addressed in Step 12.

---

## Atomic Refactoring Steps

Each step can be committed independently without breaking the application.

---

### Step 1 — Fix existing bugs (no structural change) [DONE]

**Files:** `components.js`, `indigo.css`

Changes:
- `components.js` line 269: fix `cclass=` typo → `class=` in `indigo-show-number-60`
- `indigo.css` line 110: remove invalid `red( any color )` comment inside a CSS rule
- `components.js` `indigo-select-multi-item`: the `items()` method iterates `property.itemsByLabel` for index `i` but reads from `property.items[i]` — the two arrays have different orderings; fix to filter `property.items` directly by `name.startsWith(prefix)`

---

### Step 2 — Upgrade from Bootstrap 4 to Bootstrap 5 [DONE]

**Files:** all `.html` files, `components.js`

Changes (mechanical class/attribute renames):
- Replace `data-toggle` → `data-bs-toggle`, `data-target` → `data-bs-target`, `aria-controls` targets unchanged
- Replace utility classes: `ml-*` → `ms-*`, `mr-*` → `me-*`, `mt-*`/`mb-*` unchanged, `pl-*` → `ps-*`, `pr-*` → `pe-*`
- Replace `custom-select` → `form-select`
- Replace `form-group` → remove wrapper (Bootstrap 5 removed it; use `mb-3`)
- Replace `no-gutters` → `g-0`
- Replace `float-right` → `float-end`, `float-left` → `float-start`
- Replace `btn-default` → `btn-secondary` or `btn-outline-secondary` (Bootstrap 4 had `btn-default`, Bootstrap 5 does not)
- Replace `card-block` → `card-body`
- Update `<script>` tags: drop `jquery.min.js` and `popper.min.js`; load `bootstrap.bundle.min.js` (includes Popper 2)
- Update `[data-toggle="tooltip"]` init: replace jQuery `$('[data-toggle="tooltip"]').tooltip()` in `guiSetup()` with `document.querySelectorAll('[data-bs-toggle="tooltip"]').forEach(el => new bootstrap.Tooltip(el))`

---

### Step 3 — Upgrade from Vue 2 to Vue 3 [DONE]

**Files:** `indigo.js`, `components.js`, all `.html` files

Changes in `indigo.js`:
- Replace `new Vue({ el: '#ROOT', ... })` with `const app = Vue.createApp({ ... }); app.mount('#ROOT')`
- Remove all `Vue.set(obj, key, value)` calls → replace with direct assignment `obj[key] = value` (Vue 3 Proxy tracks these)
- Remove all `Vue.delete(obj, key)` calls → replace with `delete obj[key]`
- Replace `Vue.set(INDIGO.devices, device, ...)` → `INDIGO.devices[device] = ...`
- The `INDIGO` variable becomes the component instance returned by `app.mount()` — the rest of the code that reads `INDIGO.devices` continues to work

Changes in `components.js`:
- Replace `Vue.component('name', options)` → `app.component('name', options)` (requires `app` to be defined before `components.js` is loaded, or use a deferred registration pattern)
- Alternatively: collect component definitions in `components.js` and register them in `indigo.js` after `app` is created

Changes in `.html` files:
- Drop `vue.min.js` CDN reference; add Vue 3 CDN: `<script src="vue.global.prod.js"></script>` (self-hosted)
- `v-for` loops that iterate objects now require explicit `.value` in `(value, key) in object` — audit all object `v-for` usages

---

### Step 4 — Replace dark mode with CSS custom properties [DONE]

**Files:** `indigo.css`, `components.js` (`guiSetup`, `setDarkMode`, `setLightMode`)

The current approach iterates every `div`, `input`, `canvas`, and `textarea` on every property update via jQuery class toggling. Replace with a single attribute on `<html>`:

```css
:root {
  --bg-page: #6c757d;   /* Bootstrap secondary */
  --bg-card: #f8f9fa;   /* light */
  --text-color: #212529;
  --input-bg: #fff;
}
[data-theme="dark"] {
  --bg-page: #212529;
  --bg-card: #495057;
  --text-color: #f8f9fa;
  --input-bg: #343a40;
}
```

`setDarkMode()` becomes:
```js
function setDarkMode() {
  localStorage.setItem("dark_mode", "1");
  document.documentElement.setAttribute("data-theme", "dark");
  INDIGO.dark = true;
  /* re-display Celestial if visible */
}
```

`guiSetup()` drops all jQuery class toggling; only initializes Bootstrap tooltips. Celestial's `config.background` / `config.stars.style` updates remain in `setDarkMode()` / `setLightMode()`.

---

### Step 5 — Replace jQuery-controlled status alerts with Vue `v-show` [DONE]

**Files:** `indigo.js`, all `.html` files

Currently `#SUCCESS`, `#FAILURE`, `#MESSAGE` are shown/hidden by `$('#SUCCESS').show()`. Add three reactive boolean properties to the Vue root:

```js
data: {
  ...,
  connected: false,
  failed: false,
  message: false,
}
```

In the `.html` files replace `id="SUCCESS"` / `style="display:none"` with `v-show="connected"`, etc. Remove all `$('#SUCCESS').show()` / `.hide()` calls from `indigo.js`; set the booleans instead.

---

### Step 6 — Refactor `indigo-stepper` to remove jQuery [DONE]

**Files:** `components.js`

The current template reads the input value via jQuery:
```js
@click="left($($event.target).parent().next().val())"
```

Add a `data()` property `localValue` bound with `v-model` on the `<input>`. The button click handlers read `this.localValue` instead:
```js
@click="left(localValue)"
```

---

### Step 7 — Refactor `indigo-ctrl` accordion to remove jQuery [DONE]

**Files:** `components.js`

The `openAll` / `closeAll` methods currently use jQuery to find and toggle Bootstrap 4 collapse classes. Replace with Bootstrap 5 programmatic API:
```js
openAll(id) {
  document.querySelectorAll(`#B_${id} .collapse`).forEach(el => {
    bootstrap.Collapse.getOrCreateInstance(el).show();
  });
}
```

The outer device accordion button uses `data-bs-toggle="collapse"` declaratively — no change needed there.

---

### Step 8 — Refactor `indigo-wifi-setup` to remove jQuery [DONE]

**Files:** `components.js`

Replace jQuery `$('#SSID').val()` / `$('#MODE').val()` / `$('#PASSWORD')` references with Vue `data()` properties:
```js
data() {
  return { mode: '', ssid: '', password: '' };
}
```

Bind them with `v-model` in the template. The `set()` method reads `this.ssid` and `this.password`. The `onChange` method sets `this.ssid` and `this.password` from the property items directly.

---

### Step 9 — Refactor guider button state to Vue reactive data [DONE]

**Files:** `guider.html`

The calibrate/guide button state is controlled by:
```js
$("#calibrate_button").removeClass("idle-state").addClass("busy-state");
```

Add a Vue reactive property to the page's data or use a local script variable watched by Vue:
```js
data: { calibrating: false, guiding: false }
```

Replace `id="calibrate_button"` with `:class="calibrating ? 'busy-state' : 'idle-state'"`. The `onUpdateProperty` handler sets `calibrating = false` instead of jQuery.

Since the guider page does not use a separate Vue instance (it uses the shared `INDIGO` root), the cleanest approach is to add these as Vue data properties on the INDIGO instance, or (preferred after Step 3) move them into `guider.html`'s page-level script as variables watched via `watch:` on the property.

---

### Step 10 — Refactor mount motion button state to remove jQuery [DONE]

**Files:** `mount.html`

The `moveEvent` / `stopEvent` functions traverse `event.target` via jQuery to find the button element and toggle classes. Replace with native DOM:
```js
function moveEvent(event) {
  const btn = event.currentTarget;
  btn.classList.replace('idle-state', 'busy-state');
}
function stopEvent(event) {
  const btn = event.currentTarget;
  btn.classList.replace('busy-state', 'idle-state');
}
```

`event.currentTarget` is always the element that has the `@mousedown` handler (the `<button>`), regardless of whether the click landed on `<svg>` or `<path>` inside it.

---

### Step 11 — Extract `indigo-sky-map` Vue component [DONE]

**Files:** `components.js`, `mount.html`

Create a new `indigo-sky-map` component that encapsulates all Celestial.js state.

Props:
```js
props: {
  currentCoordinates: Array,   // [raDeg, dec]
  targetCoordinates: Array,
  objectCoordinates: Array,
  geoCoordinates: Object,      // { latitude, longitude }
  zoomLevel: { type: Number, default: 4 }
}
```

Emits: `select-object` (payload: `{ ra, dec }`)

`data()` owns: `celestialConfig` (the current `config` object), `initialized: false`

`mounted()` calls `Celestial.display(this.celestialConfig)` and attaches the canvas `mousedown` → emits `select-object`.

`watch` on `currentCoordinates` / `targetCoordinates` / `objectCoordinates` triggers `Celestial.rotate()` and `Celestial.redraw()` (replaces the global `updateMap()` function and the `follow` logic).

`watch` on `zoomLevel` calls `Celestial.display()` with the updated config.

In `mount.html`:
- Replace the `<div id="map">` block, zoom buttons, and `followMarker` button with `<indigo-sky-map>` bound to Vue data
- Move `currentCoordinates`, `targetCoordinates`, `objectCoordinates` from global JS variables into the `INDIGO` data object (or a page-level reactive object)
- Connect `@select-object` to `selectObject()` which updates the RA/DEC inputs

---

### Step 12 — Replace D3 `__data__` access in catalog search [DONE]

**Files:** `components.js` (`indigo-query-db`), `mount.html`

The current search iterates `$(container).children('.star')` and reads `.\_\_data\_\_` from D3 DOM bindings. This is fragile (D3 internals) and couples the component to Celestial's rendering.

Replace with a parallel in-memory catalog loaded from the same JSON files:

```js
let starCatalog = [];   // loaded once
let dsoCatalog = [];

fetch('/data/stars.json')
  .then(r => r.json())
  .then(geojson => {
    starCatalog = geojson.features.map(f => ({
      id: f.id,
      name: f.properties.name,
      desig: f.properties.desig,
      ra: deg2h(f.geometry.coordinates[0]),
      dec: f.geometry.coordinates[1]
    }));
  });
// similar for dsos.json
```

`indigo-query-db` searches `starCatalog` and `dsoCatalog` instead of the DOM. The `container` prop is no longer needed. This also means the component works even before the Celestial map is visible.

---

### Step 13 — Extract `indigo-guider-graph` Vue component [DONE]

**Files:** `components.js`, `guider.html`

Create a component that owns the canvas and graph data arrays.

Props: none (it manages its own data)
Method: `push(driftRa, driftDec, corrRa, corrDec, rmse)` — called by the parent page's `onUpdateProperty`

`data()`: `raDrift: [], decDrift: [], raCorr: [], decCorr: [], rmse: []`

`watch` on data arrays → calls `paintGraph()` as a method on the component, using `this.$refs.driftCanvas` and `this.$refs.corrCanvas` instead of `document.getElementById`.

In `guider.html`: replace the two raw `<canvas>` elements with `<indigo-guider-graph ref="graph">`, and replace the global arrays + `paintGraph()` calls with `this.$refs.graph.push(...)`.

---

### Step 14 — Extract `indigo-status-button` component [DONE]

**Files:** `components.js`, `mount.html`, `guider.html`

The park, home, LX200 server, tracking, calibrate, and guide buttons all follow the same pattern:

```html
<button v-if="prop.state=='Ok' && prop.item('X').value" class="ok-state" ...>
<button v-else-if="prop.state=='Busy'..." class="busy-state" ...>
<button v-else class="idle-state" ...>
```

Extract a reusable component:

```js
Vue.component('indigo-status-button', {
  props: {
    property: Object,
    activeItem: String,
    activeState: { type: String, default: 'Ok' },
    action: Function
  },
  computed: {
    stateClass() {
      if (!this.property) return 'idle-state';
      if (this.property.state === this.activeState && this.property.item(this.activeItem)?.value)
        return 'ok-state';
      if (this.property.state === 'Busy') return 'busy-state';
      if (this.property.state === 'Alert') return 'alert-state';
      return 'idle-state';
    }
  },
  template: `<button class="btn btn-svg" :class="stateClass" @click="action"><slot/></button>`
});
```

---

### Step 15 — Extract shared navbar into `indigo-navbar` component [DONE]

**Files:** `components.js`, all `.html` files

All five pages share identical navbar HTML (~20 lines). The only page-specific part is the active icon. Extract to:

```js
Vue.component('indigo-navbar', {
  props: { active: String, title: String, icon: String },
  ...
})
```

Usage: `<indigo-navbar active="mount" title="INDIGO Mount" icon="mount.png"/>`

---

### Step 16 — Extract shared status bar into `indigo-status-bar` component

**Files:** `components.js`, all `.html` files

All pages share the `#SUCCESS`/`#FAILURE`/`#MESSAGE` alerts and the copyright footer. After Step 5 makes these Vue-reactive, extract them into a single component that renders all four alerts and the copyright/theme-toggle row.

---

### Step 17 — UI modernization pass

**Files:** `indigo.css`, all `.html` files

- Replace the `badge` pattern in `indigo-show-number` / `indigo-show-number-60` / `indigo-show-text` with a cleaner pill/chip design using CSS custom properties from Step 4
- Use `gap` on flex containers instead of per-child `mr-*`/`ml-*` margins
- Replace hardcoded `style="min-width:360px"` with a CSS class
- Ensure the mount page sky map fills the viewport height on desktop (`height: calc(100vh - 120px)`) and is square on mobile
- Replace `<a class="input-group-prepend">` (anchor used as a non-link wrapper) with `<div class="input-group-prepend">`
- Remove `language="javascript"` attribute from `<script>` tags (obsolete)

---

## Dependency Summary After Refactoring

| Library | Before | After |
|---------|--------|-------|
| jQuery | Required | Removed |
| Bootstrap | 4 (jQuery-dependent) | 5 (standalone) |
| Vue | 2 | 3 (Options API) |
| D3 | v5 (via Celestial) | v5 (unchanged) |
| `celestial.min.js` | Yes | Yes (encapsulated) |
| `popper.min.js` | Separate CDN | Bundled in Bootstrap 5 |

## Recommended Step Order

Steps can be batched into three commits for review:

1. **Bugs + deps** (Steps 1–3): Fix bugs, upgrade Bootstrap 5, upgrade Vue 3 — each is independently testable
2. **Reactivity** (Steps 4–10): Eliminate all jQuery DOM touches, replace with Vue reactive data
3. **Components** (Steps 11–17): Extract sky map, graph, status button, navbar, status bar; modernize CSS
