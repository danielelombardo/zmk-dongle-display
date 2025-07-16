# Dongle Display

This module repository provides a ZMK shield that replaces the built-in status screen with a custom screen designed for 128x32-pixel OLED displays.

## Usage

To use this module, first add it to your `config/west.yml` by adding a new entry to `remotes` and `projects`:

```yaml west.yml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: danielelombardo
      url-base: https://github.com/danielelombardo
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-dongle-display
      remote: danielelombardo
      revision: main
  self:
    path: config
```

Next, replace the built-in status screen by adding `dongle_display` to your `build.yaml`:

```yaml build.yaml
---
include:
  - board: nice_nano_v2
    shield: sweep_central_dongle dongle_display
```

This shield assumes that the [dongle](https://zmk.dev/docs/development/hardware-integration/dongle) is already set up and functioning with the built-in status screen.
For setup examples, refer to the shields in my [`zmk-config`](https://github.com/englmaxi/zmk-config/tree/master/boards/shields).

## Widgets
- active modifiers
- highest layer name
- peripheral battery levels
- output status

## Alternatives
- [Prospector](https://github.com/carrefinho/prospector) by @carrefinho
- [YADS](https://github.com/janpfischer/zmk-dongle-screen) by @janpfischer
