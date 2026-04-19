# PebbleTextHealth

This face offers a slight twist on word time in a simple, graphic design.  I'm a bit pedantic about word time and for 1-9 minutes past the hour, the minutes are "oh one", etc. Also, for the top of the hour the minutes displays "o'clock".   It also includes step information in an outer ring (up to 30k Steps!).  The first 10k steps are in white, 10-20k are in green (you made 10k+), and 20-30k are in purple (you overachiever).

## Supported Platforms

Targets all seven Pebble platforms: aplite, basalt, chalk, diorite, emery, flint, and gabbro.

## Code Design

### Layout

All per-platform geometry is centralised in a single `LayoutConfig` struct, populated once at window load by `layout_config_init()`. This keeps every platform-specific branch in one place — the rest of the code reads from the struct and has no platform conditionals.

Font size and text layer height scale proportionally with display width across four distinct sizes (BOLD_38 at 144px, BOLD_43 at 180px, BOLD_46 at 200px and above). On round displays (chalk, gabbro) a `corner_inset` derived from `bounds.size.w / 9` keeps text clear of the curved bezel.

### Step Ring

The step ring is rendered by two layers: a drawing layer (`step_update_proc`) that fills radial arcs, and a mask layer (`step_mask_update_proc`) that blacks out the bezel edge and the watch face interior so the ring appears as a clean border. On rectangular displays the mask uses rounded-rect primitives; on round displays it uses circles. Both layers are compiled out entirely on platforms without health service (`#if defined(PBL_HEALTH)`). The green and purple arcs are compiled out on monochrome platforms (`#if defined(PBL_COLOR)`).

### Time in Words

Time conversion lives in `num2words-en.c`, which splits the output across three text layers so each word can be sized and positioned independently. All string operations are bounds-checked using `strncat` with a tracked remaining-space counter.

### Platform Conditionals

| Define | Platforms |
|---|---|
| `PBL_HEALTH` | basalt, chalk, diorite, emery, flint, gabbro |
| `PBL_COLOR` | basalt, chalk, emery, gabbro |
| `PBL_ROUND` | chalk, gabbro |
