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

UI rules:
- Dropdowns must use text labels only. Do not put icons in dropdown controls or dropdown option values; star-count dropdowns use labels `1 star`, `2 stars`, … `8 stars`.
- All dropdown-style property selectors must use the shared Bootstrap dropdown component pattern used by `indigo-number-dropdown` / `indigo-feature-number-dropdown`, not native `<select>` elements. Tooltips should be attached to the dropdown wrapper and default to the property label unless a specific `tooltip` prop is supplied.
- Catalog search results must look clickable: use a dedicated result row style with pointer cursor, hover/focus feedback, and a small action indicator.
- The mount sky map panel must stay square (`aspect-ratio: 1 / 1`), use a fixed square scroll viewport around Celestial's internally sized map content so both horizontal and vertical scrolling work, and place map controls such as zoom in, zoom out, and center at marker as top-left overlays on the map.
- `guiSetup()` must initialize Bootstrap tooltips both immediately and after the next Vue render tick, so controls inserted by `v-if` after a property change receive working tooltips.
- `guiSetup()` and theme helpers must be safe to call before the Vue root has been assigned to `INDIGO`; update DOM theme state unconditionally, but write `INDIGO.dark` only when `INDIGO != null`.

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

### Step 16 — Extract shared status bar into `indigo-status-bar` component [DONE]

**Files:** `components.js`, all `.html` files

All pages share the `#SUCCESS`/`#FAILURE`/`#MESSAGE` alerts and the copyright footer. After Step 5 makes these Vue-reactive, extract them into a single component that renders all four alerts and the copyright/theme-toggle row.

---

### Step 17 — UI modernization pass [DONE]

**Files:** `indigo.css`, all `.html` files

- Replace the `badge` pattern in `indigo-show-number` / `indigo-show-number-60` / `indigo-show-text` with a cleaner pill/chip design using CSS custom properties from Step 4
- Use `gap` on flex containers instead of per-child `mr-*`/`ml-*` margins
- Replace hardcoded `style="min-width:360px"` with a CSS class
- Ensure the mount page sky map fills the viewport height on desktop (`height: calc(100vh - 120px)`) and is square on mobile
- Replace `<a class="input-group-prepend">` (anchor used as a non-link wrapper) with `<div class="input-group-prepend">`
- Remove `language="javascript"` attribute from `<script>` tags (obsolete)

---

### Step 18 — Route imager preview actions through `AGENT_START_PROCESS` [DONE]

**Files:** `imager.html`

`preview1()` and `preview()` currently start camera preview by changing `CCD_UPLOAD_MODE`, `CCD_PREVIEW`, and `CCD_EXPOSURE` directly. The imager agent already exposes the same operations through `AGENT_START_PROCESS`:

- `PREVIEW_1` starts `preview_1_process`
- `PREVIEW` starts `preview_process`

Update the imager UI so the preview buttons call:

- `changeProperty("Imager Agent", "AGENT_START_PROCESS", { "PREVIEW_1": true })`
- `changeProperty("Imager Agent", "AGENT_START_PROCESS", { "PREVIEW": true })`

Remove the direct `CCD_EXPOSURE` start path from these UI functions and align preview button state with the agent process state instead of local jQuery class toggles.

Source references: `indigo_drivers/agent_imager/indigo_agent_imager.c` defines `AGENT_IMAGER_START_PREVIEW_1_ITEM` / `AGENT_IMAGER_START_PREVIEW_ITEM` and dispatches them in the `AGENT_START_PROCESS` handler.

---

### Step 19 — Add imager focus estimator and star selection controls [DONE]

**Files:** `imager.html`, `components.js`

Add UI controls in the imager focuser/agent settings area for:

- `AGENT_IMAGER_FOCUS_ESTIMATOR` as a switch-property combo box
- `AGENT_IMAGER_SELECTION.COUNT` as a star count numeric input with text labels `1 star`, `2 stars`, … `8 stars`
- `AGENT_IMAGER_SELECTION.RADIUS` as a detection radius numeric input next to star count

Use the existing `indigo-select-item` / `indigo-edit-number` patterns where they fit. `AGENT_IMAGER_SELECTION.COUNT` changes the number of `X`/`Y` star-selection items on the agent side, so the UI must tolerate the property being redefined after the value changes.

Source references: `indigo_agent_imager.c` initializes `AGENT_IMAGER_FOCUS_ESTIMATOR` with `U_CURVE`, `HFD_PEAK`, `RMS_CONTRAST`, and `BAHTINOV`; it initializes `AGENT_IMAGER_SELECTION.COUNT` as maximum star count and `AGENT_IMAGER_SELECTION.RADIUS` as the detection radius in pixels.

---

### Step 20 — Show only focus settings relevant to the selected estimator [DONE]

**Files:** `imager.html`, `components.js`

Replace the current obsolete `AGENT_IMAGER_FOCUS.INITIAL` / `FINAL` controls with estimator-aware controls matching the SwiftUI client logic.

In the web GUI, keep only the focuser selector visible until a real focuser is selected. Once a focuser is selected, show the focuser position/step input and editable focuser speed before the focus-estimator selector.

Visibility should then follow the selected estimator:

- `U_CURVE`: show `AGENT_IMAGER_SELECTION.COUNT` and `AGENT_IMAGER_SELECTION.RADIUS`; show `AGENT_IMAGER_SELECTION.SUBFRAME` only when `COUNT == 1`; show `U_CURVE_SAMPLES` and `U_CURVE_STEP`
- `HFD_PEAK`: show `AGENT_IMAGER_SELECTION.RADIUS`, `AGENT_IMAGER_SELECTION.SUBFRAME`, `ITERATIVE_INITIAL`, and `ITERATIVE_FINAL`
- `RMS_CONTRAST`: show `ITERATIVE_INITIAL` and `ITERATIVE_FINAL`
- `BAHTINOV`: show `ITERATIVE_INITIAL`, `ITERATIVE_FINAL`, and `BAHTINOV_SIGMA`
- For any selected estimator, show shared autofocus controls `BACKLASH` and `BACKLASH_OVERSHOOT_FACTOR` immediately after the focus-estimator selector

Use labeled menu values for automatic subframing: `Off`, `1x`, `2x`, `3x`, `4x`, and `5x`. If the visible edit controls end on a half row, insert a row break before the read-only focuser values so `POSITION`, `TEMPERATURE`, `COMPENSATION`, and focus quality can sit together on one row. Focus quality comes from `AGENT_IMAGER_STATS`: `HFD` for `U_CURVE` and `HFD_PEAK`, `RMS_CONTRAST` for `RMS_CONTRAST`, and `BAHTINOV_ERROR` for `BAHTINOV`.

Do not show obsolete `INITIAL` / `FINAL`. Treat `BRACKETING_STEP`, `STACK`, `REPEAT`, and `DELAY` separately from the estimator UI because they are not shown in the matching SwiftUI autofocus form.

Source references: `indigo_agent_imager.c` maps `AGENT_IMAGER_FOCUS_ESTIMATOR` to `use_ucurve_focusing`, `use_iterative_focusing`, `use_hfd_estimator`, `use_rms_estimator`, and `use_bahtinov_estimator`; the SwiftUI client shows the same estimator-dependent selection and focus settings.

---

### Step 21 — Refactor imager download and focuser step actions to remove jQuery

**Files:** `imager.html`, `components.js`

Refactor the remaining imager helper actions that still depend on jQuery-only DOM access:

- `download()` should not toggle `#download_button` classes with jQuery. Move the download button state to Vue-reactive data or an `indigo-status-button`/status-class binding driven by `downloadInProgress` and the download property state.
- `focuser_in()` and `focuser_out()` should not read the step count with `$("#steps").val()`. Make the step-count input Vue-controlled, e.g. with reactive data or a Vue ref, and pass the parsed value from that state.
- Keep the existing command sequence for file download/delete and focuser motion (`FOCUSER_ON_POSITION_SET`, `FOCUSER_DIRECTION`, `FOCUSER_STEPS`) unchanged.
- Preserve current button layout and tooltips while removing the jQuery dependency from these functions.

Current source references: `imager.html` uses `downloadInProgress`, `downloadFileName`, `AGENT_IMAGER_DOWNLOAD_FILE`, `AGENT_IMAGER_DELETE_FILE`, and the shared `#steps` input for focuser position/in/out step values.

---

### Step 22 — Add imager dithering and meridian pause drop downs [DONE]

**Files:** `imager.html`, `components.js`

Add a single imager UI dropdown for dithering cadence with these user-facing options:

- Don't dither
- Dither every frame
- Dither every 2nd frame
- Dither every 3rd frame
- Dither every 4th frame
- Dither every 5th frame

Map the dropdown to the existing agent properties:

- Don't dither: set `AGENT_PROCESS_FEATURES.ENABLE_DITHERING` to `false` and `AGENT_IMAGER_BATCH.FRAMES_TO_SKIP_BEFORE_DITHER` to `-1`
- Dither every frame: set `ENABLE_DITHERING` to `true` and `FRAMES_TO_SKIP_BEFORE_DITHER` to `0`
- Dither every 2nd frame: set `ENABLE_DITHERING` to `true` and `FRAMES_TO_SKIP_BEFORE_DITHER` to `1`
- Dither every 3rd frame: set `ENABLE_DITHERING` to `true` and `FRAMES_TO_SKIP_BEFORE_DITHER` to `2`
- Dither every 4th frame: set `ENABLE_DITHERING` to `true` and `FRAMES_TO_SKIP_BEFORE_DITHER` to `3`
- Dither every 5th frame: set `ENABLE_DITHERING` to `true` and `FRAMES_TO_SKIP_BEFORE_DITHER` to `4`

Keep `AGENT_PROCESS_FEATURES.DITHER_AFTER_LAST_FRAME` separate from this dropdown; it controls whether a final-frame dither is allowed, not the cadence.

Add a second imager UI dropdown for pause on meridian transit with these user-facing options:

- Don't pause at meridian
- Pause at meridian
- Pause 15 min after meridian
- Pause 30 min after meridian
- Pause 1 hour after meridian
- Pause 2 hours after meridian
- Pause 15 min before meridian
- Pause 30 min before meridian
- Pause 1 hour before meridian
- Pause 2 hours before meridian

Map the pause-on-meridian dropdown to the existing agent properties:

- Don't pause at meridian: set `AGENT_PROCESS_FEATURES.PAUSE_AFTER_TRANSIT` to `false` and `AGENT_IMAGER_BATCH.PAUSE_AFTER_TRANSIT` to `0`
- Pause at meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `0`
- Pause 15 min after meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `0.25`
- Pause 30 min after meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `0.5`
- Pause 1 hour after meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `1`
- Pause 2 hours after meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `2`
- Pause 15 min before meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `-0.25`
- Pause 30 min before meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `-0.5`
- Pause 1 hour before meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `-1`
- Pause 2 hours before meridian: set `PAUSE_AFTER_TRANSIT` feature to `true` and batch `PAUSE_AFTER_TRANSIT` to `-2`

Source references: `indigo_agent_imager.c` initializes `AGENT_IMAGER_BATCH.FRAMES_TO_SKIP_BEFORE_DITHER` with range `-1..1000` and `AGENT_PROCESS_FEATURES.ENABLE_DITHERING`; the batch loop dithers only when the feature is enabled and the frames-to-dither counter reaches zero. It also initializes `AGENT_PROCESS_FEATURES.PAUSE_AFTER_TRANSIT` and `AGENT_IMAGER_BATCH.PAUSE_AFTER_TRANSIT` with range `-2..2` hours.

---

### Step 23 — Add autofocus graph [DONE]

**Files:** `imager.html`, `components.js`, `indigo.css`

Add an autofocus graph over the preview image showing focus quality over focuser position during autofocus. The graph should be Vue-owned and should not depend on jQuery DOM updates.

- Start/reset the graph when `AGENT_START_PROCESS.FOCUSING` starts.
- Append samples from `AGENT_IMAGER_STATS.FOCUS_POSITION`.
- Replace the previous sample when a new sample arrives for the same focuser position.
- Select the Y-axis metric from the active focus estimator:
  - `U_CURVE` / `HFD_PEAK`: `AGENT_IMAGER_STATS.HFD`
  - `RMS_CONTRAST`: `AGENT_IMAGER_STATS.RMS_CONTRAST`
  - `BAHTINOV`: `AGENT_IMAGER_STATS.BAHTINOV_ERROR`
- Keep the graph hidden unless both camera and focuser are selected.
- Place the graph in the upper-right corner of the preview image as a `256x128` overlay and keep the histogram at its original `256x128` size in the upper-left corner.
- Draw vertical sample lines, the focus-quality curve, a red marker for the current focuser position, and a green marker for the best sampled focuser position. Use readable strokes (`1px` samples, `2px` curve) so the graph remains clear over the preview.
- Preserve existing autofocus controls and button state behavior.

Source references: `indigo_agent_imager.c` updates `AGENT_IMAGER_STATS.FOCUS_POSITION`, `HFD`, `RMS_CONTRAST`, and `BAHTINOV_ERROR` while processing autofocus frames.

---

### Step 24 — Mark selected stars on the image [DONE]

**Files:** `imager.html`, `components.js`, `indigo.css`

Overlay markers for the selected autofocus stars on the preview image.

- Read the selected star count from `AGENT_IMAGER_SELECTION.COUNT`.
- Read star coordinates from the repeated `AGENT_IMAGER_SELECTION.X` / `Y` items.
- Show multiple selected stars only for `U_CURVE`; other focus estimators should mark only the first selected star.
- For `BAHTINOV`, ignore selected star markers and draw the detected mask spikes from `AGENT_IMAGER_SPIKES.RHO_1` / `THETA_1`, `RHO_2` / `THETA_2`, and `RHO_3` / `THETA_3`.
- Use `AGENT_IMAGER_SELECTION.RADIUS` for marker size.
- Hide markers for empty coordinates and when no camera is selected.
- Scale marker positions to the displayed image size so they stay aligned after image load, resize, or responsive layout changes.
- Keep the overlay Vue-owned and avoid direct jQuery access to image or marker DOM.

Source references: `indigo_agent_imager.c` writes selected star coordinates back to `AGENT_IMAGER_SELECTION` after star detection and updates the property during autofocus processing.

---

### Step 25 — Add image preview to guider GUI [DONE]

**Files:** `guider.html`, `components.js`

Show a live camera preview in the guider in the same way as in the imager GUI:

- Reuse the same `CCD_PREVIEW_IMAGE` / `CCD_PREVIEW_HISTOGRAM` property update path already used in `imager.html`.
- Show the preview image and histogram overlay (upper-left, `256×128`) inside the same `position-relative` card wrapper used in the imager.
- Overlay selected guider star markers from `AGENT_GUIDER_SELECTION`: pass the marker count into `indigo-star-selection-overlay`, read repeated `X` / `Y` coordinates and `RADIUS` for marker size, and scale to displayed image dimensions the same way as the imager. Show all selected stars for `SELECTION` and `WEIGHTED_SELECTION` detection modes (use `guiderDetectionModeIs()`); hide for `DONUTS` and `CENTROID`.
- Hide the preview card when no camera is selected (i.e. `FILTER_CCD_LIST` first item is selected).
- Add `PREVIEW_1` (single frame) and `PREVIEW` (continuous) buttons to the command row before the calibrate button, using `guiderStartProcessButtonClass()` / `guiderStartProcessButtonDisabled()` — the same coloring and disabling logic as the imager. Enable `CCD_PREVIEW` with histogram inside `guiderPreview1()` and `guiderPreview()`, as well as inside `calibrate()` and `guide()`.
- Implement `guiderStartProcessProperty()`, `guiderStartProcessSelectedItem()`, `guiderStartProcessBusy()`, `guiderStartProcessButtonClass()`, `guiderStartProcessButtonDisabled()`, and `guiderSetStartProcessItem()` helper functions analogous to the imager equivalents.

Source references: `indigo_agent_guider.c` uses `AGENT_GUIDER_SELECTION.X` / `Y` / `RADIUS` for the selected guide star position; `AGENT_START_PROCESS` items `PREVIEW_1` and `PREVIEW` trigger the corresponding preview processes.

---

### Step 26 — Move guider graphs into image preview as bottom overlay [DONE]

**Files:** `guider.html`, `components.js`, `indigo.css`

Move the drift and correction graph canvases from their current standalone position into the preview image card as a bottom-edge overlay:

- Place the `indigo-guider-graph` component absolutely at the bottom of the `position-relative` preview card, spanning its full width, with a fixed height (e.g. `128px`).
- Use a semi-transparent dark background so the graph is readable over the preview image.
- Draw guider graph axes/grid and RA/Dec data lines at `2px` stroke width for readability over the preview.
- Keep the graph hidden when no camera is selected.
- Keep the graph hidden for `PREVIEW_1` and `PREVIEW`; show it when `CALIBRATION` or `GUIDING` starts and leave it visible after those processes finish.
- Remove the graph's previous standalone card/container from the layout so the right-side column is used entirely for the preview image.

---

### Step 27 — Add capture mode and exposure dropdowns to guider camera section [DONE]

**Files:** `guider.html`

In the camera card of the guider UI, add:

- **Capture mode** (`CCD_MODE`) as an `indigo-select-item` dropdown with `:cls="'w-50'"`, matching the imager pattern.
- **Exposure** as an `indigo-number-dropdown` targeting `AGENT_GUIDER_SETTINGS.EXPOSURE` with preset values `[0, 0.1, 0.2, 0.3, 0.5, 1, 2, 3, 5, 10]`, also `:cls="'w-50'"` so it sits next to the capture mode dropdown in the same row.

Place both controls after the camera selector (`FILTER_CCD_LIST`) and before any existing exposure/delay controls. If `AGENT_GUIDER_SETTINGS.EXPOSURE` already appears elsewhere in the form, remove the duplicate.

Source references: `indigo_agent_guider.c` initializes `AGENT_GUIDER_SETTINGS` with `EXPOSURE`, `DELAY`, `STEP`, `BACKLASH`, `AGGRESSION`, `STACK`, `CALIBRATION_STEP_COUNT`, `GUIDE_RATE_RA`, and `GUIDE_RATE_DEC`.

---

### Step 28 — Add guiding rate RA / DEC controls to guider section [DONE]

**Files:** `guider.html`

In the guider device card, after the guider selector (`FILTER_GUIDER_LIST`), show the selected guider device's `GUIDER_RATE` as two side-by-side editable number inputs when the property is available:

- `GUIDER_RATE.RATE` with icon `glyphicons-resize-horizontal` and tooltip `'Guiding rate RA'`, `:cls="'w-50'"`.
- `GUIDER_RATE.DEC_RATE` with icon `glyphicons-resize-vertical` and tooltip `'Guiding rate Dec'`, `:cls="'w-50'"`.

Use `indigo-edit-number` for both. If the selected guider exposes only `GUIDER_RATE.RATE`, disable the Dec input and display the same `RATE` value there.

Source references: `indigo_names.h` defines `GUIDER_RATE.RATE` as the common/RA guide rate and optional `GUIDER_RATE.DEC_RATE` as the Dec guide rate; `indigo_docs/PROPERTIES.md` documents `DEC_RATE` as optional.

---

### Step 29 — Add drift detection section to guider GUI [DONE]

**Files:** `guider.html`

Shared visibility rule for the guider side panel: cards added in Steps 29–34 are shown only when both a real guider camera and a real guider device are selected.

Add a dedicated drift detection card in the guider side panel containing:

- **Detection mode + star count** on the same row, each at `w-50`:
  - `AGENT_GUIDER_DETECTION_MODE` as an `indigo-select-item` dropdown with `:cls="'w-50'"`. Use `:item-labels="{ SELECTION: 'Selection detection mode', WEIGHTED_SELECTION: 'Weighted selection detection mode', DONUTS: 'Donuts detection mode', CENTROID: 'Centroid detection mode' }"` instead of raw item labels.
  - `AGENT_GUIDER_SELECTION.COUNT` (`AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME`) as an `indigo-number-dropdown` with `:cls="'w-50'"`, `v-if` item check, and the same text-labeled star-count values as in the imager (`1 star`, `2 stars`, … `8 stars`). Disable the dropdown (`:disabled`) and force `:display-value` to `1` when the active detection mode is not `SELECTION` or `WEIGHTED_SELECTION` — the same pattern used for `U_CURVE` star count in the imager.

- **Detection radius + automatic subframing** on the next row, each at `w-50`, visible only when the active detection mode is `SELECTION` or `WEIGHTED_SELECTION` (use `v-if` checking `guiderDetectionModeIs('SELECTION') || guiderDetectionModeIs('WEIGHTED_SELECTION')`):
  - `AGENT_GUIDER_SELECTION.RADIUS` as an `indigo-edit-number` with preset values `[3, 6, 12, 18, 24, 48]`, icon `glyphicons-target`, and tooltip `'Detection radius'`.
  - `AGENT_GUIDER_SELECTION.SUBFRAME` as an `indigo-edit-number` with values `[{ label: 'Off', value: 0 }, 1, 2, 3, 4, 5]`, icon `glyphicons-crop`, and tooltip `'Automatic subframing'`.

Add a page-level helper function `guiderDetectionModeIs(name)` analogous to `imagerFocusEstimatorIs()` in `imager.html`, reading `AGENT_GUIDER_DETECTION_MODE_PROPERTY`.

Source references: `indigo_agent_guider.c` defines `AGENT_GUIDER_DETECTION_MODE_PROPERTY` with items `SELECTION` (index 0), `WEIGHTED_SELECTION` (index 1), `DONUTS` (index 2), and `CENTROID` (index 3). `AGENT_GUIDER_SELECTION_PROPERTY` items: `RADIUS` (index 0), `SUBFRAME` (index 1), `COUNT`/`AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME` (index 11). Star count and radius are used only by the two selection-based detection modes.

---

### Step 30 — Add RA correction mode section to guider GUI [DONE]

**Files:** `guider.html`

Add a dedicated RA correction mode card in the guider side panel. The correction mode dropdown is always visible; the fields below it change according to the selected mode.

- **RA correction mode** (`AGENT_GUIDER_CORRECTION_MODE_RA`) as an `indigo-select-item` dropdown, full width (no `:cls`). Prefix every option label with `RA ` using `:label-prefix="'RA '"` and suffix it with ` correction mode` using `:label-suffix="' correction mode'"` so the selected correction axis and context are explicit.

Conditionally show the following `indigo-edit-number` fields from `AGENT_GUIDER_SETTINGS_PROPERTY` immediately below, using `v-if` checking `guiderRaCorrectionModeIs('PI')` etc.:

- **P/I** (`AGENT_GUIDER_CORRECTION_MODE_RA` item name `PI_CONTROLLER`, accepted in page helpers as `PI`):
  - `AGGRESSIVITY_RA` — RA aggressivity (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`
  - `I_GAIN_RA` — RA integral gain, icon `glyphicons-refresh`, `:cls="'w-50'"`

- **Hysteresis** (`HYSTERESIS`):
  - `HYSTERESIS_AGGRESSIVENESS_RA` — RA aggressivity (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`
  - `HYSTERESIS_HYSTERESIS_RA` — RA hysteresis (%), icon `glyphicons-history`, `:cls="'w-50'"`

- **Linear correction** (`LINEAR_TREND`):
  - `LINEAR_TREND_AGGRESSIVENESS_RA` — RA aggressivity (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`

- **Predictive PEC** (`PPEC`):
  - `PPEC_REACTIVE_GAIN_RA` — RA PPEC reactive gain (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`
  - `PPEC_PREDICTION_GAIN_RA` — RA PPEC predictive gain (%), icon `glyphicons-signal`, `:cls="'w-50'"`
  - `PPEC_PERIOD_RA` — RA PPEC period (s), icon `glyphicons-stopwatch`, `:cls="'w-50'"`

Add a page-level helper `guiderRaCorrectionModeIs(name)` reading `AGENT_GUIDER_CORRECTION_MODE_RA_PROPERTY`, analogous to `imagerFocusEstimatorIs()`.

Source references: `indigo_agent_guider.c` defines `AGENT_GUIDER_CORRECTION_MODE_RA_PROPERTY` with item name constants `AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME` (`"PI_CONTROLLER"`), `AGENT_GUIDER_CORRECTION_MODE_HYSTERESIS_ITEM_NAME` (`"HYSTERESIS"`), `AGENT_GUIDER_CORRECTION_MODE_LINEAR_TREND_ITEM_NAME` (`"LINEAR_TREND"`), and `AGENT_GUIDER_CORRECTION_MODE_PPEC_ITEM_NAME` (`"PPEC"`). The corresponding `AGENT_GUIDER_SETTINGS` items are `AGGRESSIVITY_RA`, `I_GAIN_RA`, `HYSTERESIS_AGGRESSIVENESS_RA`, `HYSTERESIS_HYSTERESIS_RA`, `LINEAR_TREND_AGGRESSIVENESS_RA`, `PPEC_REACTIVE_GAIN_RA`, `PPEC_PREDICTION_GAIN_RA`, and `PPEC_PERIOD_RA`.

---

### Step 31 — Add Dec correction mode section to guider GUI [DONE]

**Files:** `guider.html`

Add a dedicated Dec correction mode card in the guider side panel. The layout follows the SwiftUI client logic.

**First row** — two half-width dropdowns side by side:
- `AGENT_GUIDER_DEC_MODE` (`indigo-select-item`, `:cls="'w-50'"`) — Dec guiding mode with items: `BOTH` (North & South), `NORTH` (North only), `SOUTH` (South only), `NONE`. Use `:item-labels="{ BOTH: 'Guide North and South', NORTH: 'Guide North only', SOUTH: 'Guide South only', NONE: 'Do not guide Dec' }"` instead of raw item labels.
- `AGENT_GUIDER_CORRECTION_MODE_DEC` (`indigo-select-item`, `:cls="'w-50'"`, `:label-prefix="'Dec '"`, `:label-suffix="' correction mode'"`) — Dec correction mode with items: `PI_CONTROLLER` (accepted in page helpers as `PI`), `HYSTERESIS`, `LINEAR_TREND`, `RESIST_SWITCH`. Disabled (`:disabled`) when Dec guiding mode is `NONE`.

When Dec guiding mode is `NONE`, everything below the first row is hidden (`v-if="!guiderDecModeIs('NONE')"`).

**Second row** — two half-width switch dropdowns side by side (shown only when mode ≠ `NONE`):
- `AGENT_GUIDER_APPLY_DEC_BACKLASH` (`indigo-select-item`, `:cls="'w-50'"`) — Apply Dec backlash. Disabled (`:disabled`) when Dec guiding mode is not `BOTH`. Use `:item-labels="{ DISABLED: 'Disabled Dec backlash', ENABLED: 'Enabled Dec backlash' }"` instead of generic Enabled/Disabled labels.
- `AGENT_GUIDER_FLIP_REVERSES_DEC` (`indigo-select-item`, `:cls="'w-50'"`, `:fallback-item="'ENABLED'"`) — Reverse Dec on flip (items `ENABLED`/`DISABLED`). Use the fallback item so this OneOfMany switch does not render empty if no current item is selected. Use `:item-labels="{ ENABLED: 'Reverse speed on meridian flip', DISABLED: 'Keep speed on meridian flip' }"` instead of generic Enabled/Disabled labels.

**Conditional fields** from `AGENT_GUIDER_SETTINGS_PROPERTY` below, using `v-if` checking `guiderDecCorrectionModeIs(...)`:

- **P/I** (`PI_CONTROLLER`, accepted in page helpers as `PI`):
  - `AGGRESSIVITY_DEC` — Dec aggressivity (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`
  - `I_GAIN_DEC` — Dec integral gain, icon `glyphicons-refresh`, `:cls="'w-50'"`

- **Hysteresis** (`HYSTERESIS`):
  - `HYSTERESIS_AGGRESSIVENESS_DEC` — Dec aggressivity (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`
  - `HYSTERESIS_HYSTERESIS_DEC` — Dec hysteresis (%), icon `glyphicons-history`, `:cls="'w-50'"`

- **Linear correction** (`LINEAR_TREND`):
  - `LINEAR_TREND_AGGRESSIVENESS_DEC` — Dec aggressivity (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`

- **Resist switch** (`RESIST_SWITCH`):
  - `RESIST_SWITCH_AGGRESSIVENESS_DEC` — Dec aggressivity (%), icon `glyphicons-dashboard`, `:cls="'w-50'"`
  - `RESIST_SWITCH_FAST_THRESHOLD_DEC` — Dec resist threshold (px), icon `glyphicons-flash`, `:cls="'w-50'"`

Add page-level helper functions `guiderDecModeIs(name)` reading `AGENT_GUIDER_DEC_MODE_PROPERTY` and `guiderDecCorrectionModeIs(name)` reading `AGENT_GUIDER_CORRECTION_MODE_DEC_PROPERTY`, analogous to `guiderRaCorrectionModeIs()`.

Source references: `indigo_agent_guider.c` defines `AGENT_GUIDER_DEC_MODE_PROPERTY` with items `BOTH` (0), `NORTH` (1), `SOUTH` (2), `NONE` (3); `AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY` with items `DISABLED` (0), `ENABLED` (1); `AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY` with items `ENABLED` (0), `DISABLED` (1); `AGENT_GUIDER_CORRECTION_MODE_DEC_PROPERTY` with items `PI_CONTROLLER`, `HYSTERESIS`, `LINEAR_TREND`, `RESIST_SWITCH`. Settings items: `AGGRESSIVITY_DEC`, `I_GAIN_DEC`, `HYSTERESIS_AGGRESSIVENESS_DEC`, `HYSTERESIS_HYSTERESIS_DEC`, `LINEAR_TREND_AGGRESSIVENESS_DEC`, `RESIST_SWITCH_AGGRESSIVENESS_DEC`, and `RESIST_SWITCH_FAST_THRESHOLD_DEC`.

---

### Step 32 — Add integral stack section to guider GUI [DONE]

**Files:** `guider.html`

Add a dedicated integral stack card in the guider side panel.

- **Integral stack size** (`AGENT_GUIDER_SETTINGS.STACK`) as an `indigo-number-dropdown` with labeled values:
  - `{ label: 'Integral stack size 1', value: 1 }`
  - `{ label: 'Integral stack size 2', value: 2 }`
  - `{ label: 'Integral stack size 3', value: 3 }`
  - `{ label: 'Integral stack size 4', value: 4 }`
  - `{ label: 'Integral stack size 5', value: 5 }`
  - `{ label: 'Integral stack size 10', value: 10 }`
  - `{ label: 'Integral stack size 20', value: 20 }`

- Below it, half-width `indigo-edit-number` fields:
  - `AGENT_GUIDER_SETTINGS.MIN_ERROR` — Min error (px), range 0–5, step 0.1, icon `glyphicons-resize-full`, `:cls="'w-50'"`
  - `AGENT_GUIDER_SETTINGS.MIN_PULSE` — Min pulse (s), icon `glyphicons-flash`, `:cls="'w-50'"`
  - `AGENT_GUIDER_SETTINGS.MAX_PULSE` — Max pulse (s), icon `glyphicons-flash`, `:cls="'w-50'"`

Source references: `indigo_agent_guider.c` initializes `AGENT_GUIDER_SETTINGS.STACK` (`AGENT_GUIDER_SETTINGS_STACK_ITEM`, index 19) with range `1..MAX_STACK`, step `1`, default `1`; `AGENT_GUIDER_SETTINGS.MIN_ERROR` (`AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM`, index 12) with range `0..5`, step `0.1`, default `0`; `AGENT_GUIDER_SETTINGS.MIN_PULSE` with the minimum correction pulse duration in seconds; `AGENT_GUIDER_SETTINGS.MAX_PULSE` with the maximum correction pulse duration in seconds.

---

### Step 33 — Add calibration section to guider GUI [DONE]

**Files:** `guider.html`

Add a dedicated calibration card in the guider side panel.

- **Calibration step** (`AGENT_GUIDER_SETTINGS.STEP0`) as an `indigo-edit-number` with preset values `[0.01, 0.05, 0.1, 0.2, 0.5, 1]`, icon `glyphicons-hourglass`, and tooltip `'Calibration Step'`. Full width.

- **Read-only results** on the next row — four `indigo-show-number` fields from `AGENT_GUIDER_SETTINGS_PROPERTY`, all on the same flex row with `:cls="'w-25'"`:
  - `ANGLE` — icon `'angle'`, tooltip `'Angle'`
  - `BACKLASH` — icon `'b-lash'`, tooltip `'Backlash'`
  - `SPEED_RA` — icon `'px/s α'`, tooltip `'Right Ascension Speed'`
  - `SPEED_DEC` — icon `'px/s δ'`, tooltip `'Declination Speed'`

Source references: `indigo_agent_guider.c` initializes `AGENT_GUIDER_SETTINGS.STEP0` as the calibration step size in seconds and writes back `ANGLE`, `BACKLASH`, `SPEED_RA`, and `SPEED_DEC` after a successful calibration run.

---

### Step 34 — Add dithering section to guider GUI [DONE]

**Files:** `guider.html`

Add a dedicated dithering card in the guider side panel.

**First row** — two half-width dropdowns side by side:
- **Dithering strategy** (`AGENT_GUIDER_DITHERING_STRATEGY`) as an `indigo-select-item` with `:cls="'w-50'"`. Items: `RANDOMIZED_SPIRAL`, `RANDOM`, `SPIRAL`. Use `:item-labels="{ RANDOMIZED_SPIRAL: 'Randomized spiral dithering', RANDOM: 'Random dithering', SPIRAL: 'Spiral dithering' }"` instead of raw item labels.
- **Dithering amount** (`AGENT_GUIDER_SETTINGS.DITHERING_MAX_AMOUNT`) as an `indigo-number-dropdown` with `:cls="'w-50'"` and labeled values:
  - `{ label: 'Dithering amount 1 px', value: 1 }`
  - `{ label: 'Dithering amount 2 px', value: 2 }`
  - `{ label: 'Dithering amount 3 px', value: 3 }`
  - `{ label: 'Dithering amount 5 px', value: 5 }`
  - `{ label: 'Dithering amount 10 px', value: 10 }`

**Second row** — two half-width `indigo-edit-number` fields side by side:
- `AGENT_GUIDER_SETTINGS.DITHERING_SETTLE_TIME_LIMIT` — Settle down max limit (s), icon `glyphicons-hourglass`, `:cls="'w-50'"`
- `AGENT_GUIDER_SETTINGS.DITHERING_LIMIT` — Settle down min limit (frames), icon `glyphicons-scale`, `:cls="'w-50'"`

Source references: `indigo_agent_guider.c` defines `AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY` (switch, items `RANDOMIZED_SPIRAL`/`RANDOM`/`SPIRAL`); `AGENT_GUIDER_SETTINGS.DITHERING_MAX_AMOUNT` (index 28, range 0–15, step 1, default 1, "Dithering max amount (px)"); `AGENT_GUIDER_SETTINGS.DITHERING_SETTLE_TIME_LIMIT` (index 29, range 0–300, step 1, default 60, "Dithering Settle time limit (s)"); `AGENT_GUIDER_SETTINGS.DITHERING_LIMIT` (index 30, range 1–50, step 1, default 5, "Dithering min settling limit (frames)").

---

### Step 35 — Use mount agent coordinates in mount GUI [DONE]

**Files:** `mount.html`

Update the mount side panel so target coordinates and read-only current-coordinate display use mount agent properties instead of raw mount coordinate properties.

- Move the editable target RA/Dec fields from `MOUNT_EQUATORIAL_COORDINATES.RA` / `MOUNT_EQUATORIAL_COORDINATES.DEC` to `AGENT_MOUNT_TARGET_COORDINATES.RA` / `AGENT_MOUNT_TARGET_COORDINATES.DEC`; those fields are still the target coordinates used by Slew and Sync.
- Render target and display coordinate chips only when the corresponding mount-agent coordinate property is available; hide these coordinates instead of falling back to raw/non-agent coordinate properties.
- In the location source card, show editable `GEOGRAPHIC_COORDINATES.LONGITUDE` / `LATITUDE` only when `AGENT_SITE_DATA_SOURCE.HOST` (`Use agent coordinates`) is selected.
- Place catalog search above target RA/Dec inside the mount selection card, with visually clickable result rows.
- Place `MOUNT_SLEW_RATE` and `MOUNT_TRACK_RATE` directly below target RA/Dec and above the read-only display chips.
- Update object selection, Slew/Sync coordinate preparation, and sky-map target-coordinate helpers to read/write `AGENT_MOUNT_TARGET_COORDINATES` for target RA/Dec. Slew/Sync actions themselves are routed through `AGENT_START_PROCESS` in Step 36.
- Replace the read-only current coordinate fields currently bound to `MOUNT_EQUATORIAL_COORDINATES` / `MOUNT_HORIZONTAL_COORDINATES` with `AGENT_MOUNT_DISPLAY_COORDINATES`:
  - `RA_JNOW` — current RA, sexagesimal display, icon `'α'`, tooltip `'Right Ascension JNow'`
  - `DEC_JNOW` — current Dec, sexagesimal display, icon `'δ'`, tooltip `'Declination JNow'`
  - `ALT` — current altitude, sexagesimal display, icon `'Ε'`, tooltip `'Altitude'`
  - `AZ` — current azimuth, sexagesimal display, icon `'Α'`, tooltip `'Azimuth'`
- Add additional read-only mount-agent values where they fit without crowding the first coordinate row:
  - `HA` — hour angle, sexagesimal display
  - `RISE`, `TRANSIT`, `SET` — sexagesimal display times
- Hide these read-only display fields when `AGENT_MOUNT_DISPLAY_COORDINATES` is not available. Do not fall back to `MOUNT_EQUATORIAL_COORDINATES` for current coordinates, because the mount agent computes JNow/display values consistently from site and target state.
- In the mount selection card, keep only `FILTER_MOUNT_LIST` visible until a mount is selected; hide target coordinates, current-coordinate display, action buttons, rate dropdowns, and manual controls when no mount is selected.
- In the dome section, keep only `FILTER_DOME_LIST` visible until a dome is selected; then show the coordinate chips and a fourth read-only shutter status chip showing `shutter opened`, `shutter closed`, or `N/A` based on `DOME_SHUTTER.OPENED` / `CLOSED` availability and value.
- In the GPS section, keep only `FILTER_GPS_LIST` visible until a GPS is selected; then show the coordinate chips and a read-only fix status chip showing `no fix`, `2d fix`, `3d fix`, or `N/A` based on `GPS_STATUS.NO_FIX` / `2D_FIX` / `3D_FIX`.

Source references: `indigo_names.h` defines `AGENT_MOUNT_TARGET_COORDINATES` items `RA` and `DEC` (property name string `AGENT_MOUNT_EQUATORIAL_COORDINATES`) and `AGENT_MOUNT_DISPLAY_COORDINATES` items used here (`RA_JNOW`, `DEC_JNOW`, `ALT`, `AZ`, `HA`, `RISE`, `TRANSIT`, and `SET`; property name string `AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY`); `indigo_agent_mount.c` updates these values from mount coordinates, site data, and tracking state.

---

### Step 36 — Route mount actions through agent start process [DONE]

**Files:** `mount.html`

Replace direct mount-device action writes with mount-agent `AGENT_START_PROCESS` commands.

- Slew button: write `AGENT_START_PROCESS.SLEW = true`.
- Sync button: write `AGENT_START_PROCESS.SYNC = true`.
- Park button: write `AGENT_START_PROCESS.PARK = true`.
- Unpark button: write `AGENT_START_PROCESS.UNPARK = true`.
- Home button: write `AGENT_START_PROCESS.HOME = true`.
- Tracking on button/state action: write `AGENT_START_PROCESS.TRACK_ON = true`.
- Tracking off button/state action: write `AGENT_START_PROCESS.TRACK_OFF = true`.
- Keep `AGENT_MOUNT_TARGET_COORDINATES` as the target RA/Dec source from Step 35; do not write `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_HOME`, or `MOUNT_TRACKING` directly from the web UI for these actions.
- Stop button: keep it enabled while any mount process is busy and write `AGENT_ABORT_PROCESS.ABORT = true`, with `MOUNT_ABORT_MOTION.ABORT_MOTION` only as a fallback.
- Match the imager/guider process-button convention: while `AGENT_START_PROCESS` is `Busy`, disable all mount action buttons and manual-motion controls except Stop. Keep the currently selected start-process item highlighted with busy/alert styling.
- Arrange mount controls in two rows: first row has Slew, Sync, Park, Home, and Tracking on the left with Stop on the right; second row has manual motion on the left with the LX200 server button on the right.
- Use `AGENT_MOUNT_STATE` for action state display and button coloring/busy state:
  - `SLEW` for Slew/Sync button coloring and slew status, while preserving the selected `SLEW` / `SYNC` start-process item for immediate busy/alert feedback
  - `PARK` for Park/Unpark state
  - `HOME` for Home state
  - `TRACK` for Tracking state
- Use `AGENT_START_PROCESS` item availability for Park/Unpark/Home buttons where needed, so the UI reflects agent-mediated capabilities rather than raw device switches.

Source references: `indigo_names.h` defines mount-agent start-process items `SLEW`, `SYNC`, `PARK`, `UNPARK`, `HOME`, `TRACK_ON`, and `TRACK_OFF`, and `AGENT_MOUNT_STATE` items `SLEW`, `PARK`, `HOME`, and `TRACK`; `indigo_agent_mount.c` handles those `AGENT_START_PROCESS` items, forwards the corresponding low-level `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_HOME`, and `MOUNT_TRACKING` operations, and mirrors raw mount state into `AGENT_MOUNT_STATE`.

### Step 37 — Extract `indigo-ctrl-property-body` component

**Files:** `components.js`

Extract the property items form (currently the `<form class="m-0">` inside each property's collapse body in `indigo-ctrl`) into a standalone `indigo-ctrl-property-body` component. This is a preparatory refactor required before Step 38 so that the wide-layout right panel can render property items without duplicating the template.

Props:
```js
props: { property: Object }
```

Move these methods from `indigo-ctrl` into `indigo-ctrl-property-body` (none of them reference `devices` or any parent state):
- `dirty(item)`, `format(item, value)`, `value(item)`, `newValue(item, value)`, `reset(property)`, `set(property)`, `setSwitch(property, itemName, value)`, `isAbsoluteUrl(value)`, `isImage(value)`, `localUrl(value)`
- Add `itemState(item)` (a copy of the current `state()` function scoped to item-level light states, i.e. reads `item.value.toLowerCase() + "-state"`) to render light-type buttons inside the body.

Template: the existing `<form class="m-0">` content with all `v-if="property.type == 'text'"` / `number` / `switch` / `light` / `blob` branches, unchanged.

In `indigo-ctrl`, replace the inline `<form class="m-0">…</form>` block with `<indigo-ctrl-property-body :property="property"/>`. No visual change.

---

### Step 38 — Add two-column wide-layout navigation to `indigo-ctrl`

**Files:** `components.js`, `indigo.css`

Add a second rendering mode to `indigo-ctrl` that activates at the `xl` breakpoint (≥1200 px, same as other pages). Below `xl` the existing accordion is kept unchanged.

**New reactive `data()` fields in `indigo-ctrl`:**

```js
selected: { device: null, group: null, property: null },
expandedDevices: {},   // { [deviceName]: bool }
expandedGroups: {},    // { [deviceName + '_' + groupName]: bool }
```

**New methods:**

- `selectDevice(deviceName)` — toggle `expandedDevices[deviceName]`; set `selected = { device: deviceName, group: null, property: null }` (right panel shows all groups/properties of the device).
- `selectGroup(deviceName, groupName)` — toggle `expandedGroups[deviceName+'_'+groupName]`; set `selected = { device: deviceName, group: groupName, property: null }` (right panel shows all properties of the group).
- `selectProperty(deviceName, groupName, property)` — set `selected = { device: deviceName, group: groupName, property }` (right panel shows that property's items only); ensure device and group are marked expanded.
- `isDeviceExpanded(deviceName)` — returns `!!expandedDevices[deviceName]`.
- `isGroupExpanded(deviceName, groupName)` — returns `!!expandedGroups[deviceName+'_'+groupName]`.
- `groupProperties(deviceName, groupName)` — returns the property map for that group (delegates to `groups(devices[deviceName])[groupName]`).

Add a `watch: { devices: { deep: false } }` that validates `selected` after any top-level device change, clearing `selected` fields that no longer correspond to an existing device / group / property key.

**Template structure:**

```html
<!-- Narrow: accordion, visible on <xl (existing code, unchanged) -->
<div class="d-xl-none accordion p-1 w-100">
  …existing device/group/property accordion…
</div>

<!-- Wide: two-column, visible on xl+ -->
<div class="d-none d-xl-flex w-100" style="min-height:0">

  <!-- Left tree: col-xl-4 -->
  <div class="indigo-side-panel col-xl-4 overflow-auto border-end">
    <div v-for="(device, deviceName) in devices" class="mb-1">
      <!-- Device row -->
      <div class="d-flex align-items-center px-2 py-1 ctrl-tree-device"
           :class="[state(device), { 'ctrl-tree-selected': selected.device == deviceName && !selected.group }]"
           @click="selectDevice(deviceName)" style="cursor:pointer">
        <span class="ctrl-tree-arrow me-1">{{ isDeviceExpanded(deviceName) ? '▾' : '▸' }}</span>
        {{ deviceName }}
      </div>
      <!-- Groups (shown when device expanded) -->
      <template v-if="isDeviceExpanded(deviceName)">
        <div v-for="(group, groupName) in groups(device)" class="ms-2">
          <!-- Group row -->
          <div class="d-flex align-items-center px-2 py-1 ctrl-tree-group"
               :class="{ 'ctrl-tree-selected': selected.device == deviceName && selected.group == groupName && !selected.property }"
               @click="selectGroup(deviceName, groupName)" style="cursor:pointer">
            <span class="ctrl-tree-arrow me-1">{{ isGroupExpanded(deviceName, groupName) ? '▾' : '▸' }}</span>
            {{ groupName }}
          </div>
          <!-- Properties (shown when group expanded) -->
          <template v-if="isGroupExpanded(deviceName, groupName)">
            <div v-for="(property, name) in group"
                 class="px-3 py-1 ctrl-tree-property"
                 :class="[state(property), { 'ctrl-tree-selected': selected.property === property }]"
                 @click="selectProperty(deviceName, groupName, property)" style="cursor:pointer">
              <span class="icon-indicator me-1"></span>{{ property.label }}
            </div>
          </template>
        </div>
      </template>
    </div>
  </div>

  <!-- Right panel: col-xl-8 -->
  <div class="col-xl-8 overflow-auto p-2">

    <!-- Single property selected -->
    <template v-if="selected.property">
      <div class="card mb-1">
        <div class="card-header p-2" :class="state(selected.property)">
          {{ selected.property.label }}
          <small class="float-end">{{ selected.property.name }}</small>
        </div>
        <div class="card-body p-2 bg-light">
          <indigo-ctrl-property-body :property="selected.property"/>
        </div>
      </div>
    </template>

    <!-- Group selected: all properties in that group -->
    <template v-else-if="selected.group">
      <div v-for="(property, name) in groupProperties(selected.device, selected.group)" class="card mb-1">
        <div class="card-header p-2" :class="state(property)">
          {{ property.label }}<small class="float-end">{{ name }}</small>
        </div>
        <div class="card-body p-2 bg-light">
          <indigo-ctrl-property-body :property="property"/>
        </div>
      </div>
    </template>

    <!-- Device selected: all properties of all groups -->
    <template v-else-if="selected.device">
      <template v-for="(group, groupName) in groups(devices[selected.device])">
        <h6 class="px-1 pt-2 pb-1 text-muted">{{ groupName }}</h6>
        <div v-for="(property, name) in group" class="card mb-1">
          <div class="card-header p-2" :class="state(property)">
            {{ property.label }}<small class="float-end">{{ name }}</small>
          </div>
          <div class="card-body p-2 bg-light">
            <indigo-ctrl-property-body :property="property"/>
          </div>
        </div>
      </template>
    </template>

    <!-- Nothing selected -->
    <template v-else>
      <div class="text-muted text-center p-5">Select a device, group or property</div>
    </template>

  </div>
</div>
```

**CSS additions in `indigo.css`:**

```css
.ctrl-tree-device,
.ctrl-tree-group,
.ctrl-tree-property {
  border-radius: 4px;
  transition: background 0.1s;
}
.ctrl-tree-device:hover,
.ctrl-tree-group:hover,
.ctrl-tree-property:hover {
  background: rgba(0,0,0,.06);
}
.ctrl-tree-selected {
  background: rgba(0,0,0,.12) !important;
  font-weight: 500;
}
[data-theme="dark"] .ctrl-tree-device:hover,
[data-theme="dark"] .ctrl-tree-group:hover,
[data-theme="dark"] .ctrl-tree-property:hover {
  background: rgba(255,255,255,.08);
}
[data-theme="dark"] .ctrl-tree-selected {
  background: rgba(255,255,255,.15) !important;
}
```

The `indigo-side-panel` class (already used on other pages) provides the sticky/scroll behaviour; no new layout class is needed in `ctrl.html` — only the existing `col-sm-12` wrapper changes to remove the width constraint so the `d-xl-flex` row inside can fill the full card width.

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

Steps can be batched into four commits for review:

1. **Bugs + deps** (Steps 1–3): Fix bugs, upgrade Bootstrap 5, upgrade Vue 3 — each is independently testable
2. **Reactivity** (Steps 4–10): Eliminate all jQuery DOM touches, replace with Vue reactive data
3. **Components** (Steps 11–17): Extract sky map, graph, status button, navbar, status bar; modernize CSS
4. **Functional imager UI** (Steps 18–24): Route preview through the agent, remove remaining imager action jQuery, expose estimator/selection/dithering controls, filter autofocus settings by estimator, and add autofocus image/graph feedback
5. **Guider UI** (Steps 25–34): Add live preview with star overlay, move graphs into preview overlay, add capture mode and exposure dropdowns, add guiding rate RA/Dec controls, add drift detection section with mode/star-count/radius/subframe, add RA and Dec correction mode sections with conditional sub-fields, add integral stack section with stack-size dropdown and min-error/max-pulse fields, add calibration section with step input and read-only angle/backlash/speed results, add dithering section with strategy/amount dropdowns and settle-limit inputs
6. **Mount UI** (Steps 35–36): Move target coordinates to `AGENT_MOUNT_TARGET_COORDINATES`, current-coordinate/derived status displays to `AGENT_MOUNT_DISPLAY_COORDINATES`, and mount actions to `AGENT_START_PROCESS`
7. **Control panel UI** (Steps 37–38): Extract `indigo-ctrl-property-body` component, then add two-column wide-layout navigation (tree left / content panel right) to `ctrl.html`
