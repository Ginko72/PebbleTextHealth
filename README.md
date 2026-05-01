# PebbleTextHealth

This face offers a slight twist on word time in a simple, graphic design. I'm a bit pedantic about word time and for 1-9 minutes past the hour, the minutes are "oh one", etc. Also, for the top of the hour the minutes displays "o'clock". When you flick your wrist to activate the backlight, seconds are displayed as tick marks around the rim for a duration adjustable in settings.

It also tracks your steps in an outer ring — up to three times your daily goal. The first lap fills in white as you close in on your goal; hit it and the ring resets in green for a second lap (nice work!), then purple if you make it around a third time (you overachiever). Your daily step goal and the tick mark duration are both configurable in settings.

## Settings

| Setting | Default | Range |
|---|---|---|
| Daily Step Goal | 10,000 | 1,000–20,000, step 250 |
| Tick Mark Persistence | 3 s | 3–60 s |
| Date Format | US (MM.DD) | toggle US / International (DD.MM) |

## Supported Platforms

Targets all seven Pebble platforms: aplite, basalt, chalk, diorite, emery, flint, and gabbro.

## Code Design

### Layout

All per-platform geometry is centralised in a single `LayoutConfig` struct, populated once at window load by `layout_config_init()`. Font IDs and fixed dimension constants are selected at compile time using `#if PBL_DISPLAY_WIDTH` so each platform binary contains only the code and fonts it needs. Fields that depend on screen height remain runtime calculations.

| Branch | Platforms | Fonts |
|---|---|---|
| `PBL_DISPLAY_WIDTH >= 200` | emery, gabbro | Amiko Bold 46, Regular 32/22 |
| `PBL_DISPLAY_WIDTH >= 180` | chalk | Amiko Bold 38, Regular 24/16 |
| else | aplite, basalt, diorite, flint | Amiko Bold 38, Regular 24/16 |

Font resources in `package.json` carry matching `targetPlatforms` lists so each binary only bundles the fonts it uses. On round displays (chalk, gabbro) a `corner_inset` keeps text clear of the curved bezel.

### Step Ring

The step ring is rendered by two layers: a drawing layer (`step_update_proc`) that fills radial arcs up to three laps of the user's configured step goal, and a mask layer (`step_mask_update_proc`) that blacks out the bezel edge and watch face interior so the ring appears as a clean border. On rectangular displays the mask uses rounded-rect primitives; on round displays it uses circles. Both layers are compiled out entirely on platforms without health service (`#if defined(PBL_HEALTH)`). The green and purple arcs are compiled out on monochrome platforms (`#if defined(PBL_COLOR)`).

### Time in Words

Time conversion lives in `num2words-en.c`, which splits the output across three text layers so each word can be sized and positioned independently. All string operations are bounds-checked using `strncat` with a tracked remaining-space counter.

### Settings & Configuration

User settings are persisted via `settings.h` / `settings.c` using the Pebble `persist_read/write` API. The phone-side configuration UI is built with [@rebble/clay](https://www.npmjs.com/package/@rebble/clay). Settings are received over AppMessage and applied live without restarting the watchface.

### Platform Conditionals

| Define | Platforms |
|---|---|
| `PBL_HEALTH` | basalt, chalk, diorite, emery, flint, gabbro |
| `PBL_COLOR` | basalt, chalk, emery, gabbro |
| `PBL_ROUND` | chalk, gabbro |
