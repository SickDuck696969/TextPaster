# TextPaster

Nintendo Switch text-pasting overlay by **SickDuck**. Requires Atmosphere and Ultrahand.

## Install

Extract the release ZIP to your SD root and reboot. It installs:

- `switch/.overlays/TextPaster.ovl`
- `atmosphere/contents/4200000000545053/exefs.nsp`
- `config/TextPaster/config.example.ini`

Your existing `config.ini` is preserved. A default is created on first use; older single-text configs migrate automatically.

## Use

1. Open TextPaster in Ultrahand.
2. Choose **Saved texts > Add text**, enter text, and Save.
3. Open an entry to **Use for paste**, edit, rename, or delete it. New entries are selected automatically.
4. Set **Enabled** to On, close the overlay, and open the standard Switch keyboard.
5. Press **L + ZR** (or your selected shortcut). Review the entered text and confirm manually.

Holding the shortcut does not repeat it. **Off** unloads the background worker. Shortcut options: `L+ZR`, `ZL+R`, `L+R`, `L+ZL`, `R+ZR`, `ZL+ZR`.

### Editor controls

**D-pad** select key, **A** enter, **B** cancel, **+** save, **X** delete, **Y** change layout, **L/R** move cursor. Touch is supported. Text stays visible.

## Config

Everything can be managed in the overlay. For manual edits, use `config/TextPaster/config.ini`; see [config.example.ini](config.example.ini) for the full template.

```ini
version=2
combo=L+ZR
delay_ms=40
selected=1

[text:1]
name=Greeting
text=Hello from TextPaster
```

Up to **32 texts**, each with a unique ID, a name of 1-48 characters, and text of 1-512 characters. `selected` identifies the entry to paste. Keep global settings before text sections. `delay_ms` accepts 20-200. Changes reload automatically.

Use **printable ASCII and a US keyboard layout**. This emulates typing; it does not support arbitrary Unicode or game-owned keyboards. Shortcut buttons can also affect the underlying keyboard. Text is stored plainly on the SD card.

The config folder contains only the config and example during normal use. Temporary control/recovery files live in `switch/.overlays/TextPaster.runtime` and are cleaned up after use. Reopen and close TextPaster if a crash leaves typing paused.

## Build

With devkitPro/libnx installed:

```sh
make -j2
make -C daemon -j2
```

Outputs: `TextPaster.ovl` and `daemon/daemon.nsp` (install the latter as `exefs.nsp`, not through a game installer).

Cross-compilation passes; console behavior still needs hardware verification. Host config tests require a Visual Studio developer prompt and Windows SDK: `tests\run-host-tests.cmd`.

Built with [libultrahand](https://github.com/ppkantorski/libultrahand) and the [Tesla template](https://github.com/WerWolv/Tesla-Template). Keyboard report handling references [sys-botbase](https://github.com/olliz0r/sys-botbase). Licenses are retained in the repository.
