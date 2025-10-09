## Project Overview

DeskHop enables fast desktop switching between two computers using a single keyboard/mouse. Built on dual Raspberry Pi Pico boards with PIO-based USB dual-role support. See [README.md](README.md) for complete project details.

## Build System

### Building the Firmware

**IMPORTANT**: The host machine does not have the ARM cross-compilation toolchain installed. All builds go through Docker (OrbStack/Docker Desktop) via `docker-compose`.

```shell
# Release build
docker-compose -f misc/docker.yml run --rm build_container sh -c \
  "cmake --preset=release && cmake --build --preset=release"

# Debug build
docker-compose -f misc/docker.yml run --rm build_container sh -c \
  "cmake --preset=debug && cmake --build --preset=debug"
```

**If the cmake cache is stale** (e.g. after switching between local and docker builds), clear it first:

```shell
rm -rf build/release build/debug
```

**Output files**:
- Release: `build/release/deskhop.uf2`
- Debug: `build/debug/deskhop.uf2`

**CMake presets** are defined in `CMakePresets.json`. The `release` preset enables `ENABLE_USER_OVERRIDES`. The `debug` preset additionally enables `DH_DEBUG` and `DH_DEBUG_CDC_FLASH`.

### Verifying Build Success

When checking that builds pass, test both configurations:

```shell
# Clean first if needed
rm -rf build/release build/debug

# Release
docker-compose -f misc/docker.yml run --rm build_container sh -c \
  "cmake --preset=release && cmake --build --preset=release"

# Debug
docker-compose -f misc/docker.yml run --rm build_container sh -c \
  "cmake --preset=debug && cmake --build --preset=debug"
```

Both builds must complete without errors.

### Flashing

```shell
misc/deskhop.py flash                          # flash build/release/deskhop.uf2 (default)
misc/deskhop.py flash build/debug/deskhop.uf2  # flash a specific build
misc/deskhop.py flash --debug                  # flash build/debug/deskhop.uf2
```

The script attempts to trigger bootloader mode via CDC, then waits for the `/Volumes/RPI-RP2` volume to appear, copies the file, and waits for it to disappear.

### Serial Console

```shell
misc/deskhop.py console  # auto-detect DeskHop CDC device and open interactive terminal
```

### Rebuild Web Configuration UI

```shell
cd webconfig/
./render.py  # Requires jinja2: pip install jinja2
```

### Debug Feature Flags

These are toggled via cmake cache variables (already wired into the `debug` preset):

| Flag | Preset | Purpose |
|------|--------|---------|
| `DH_DEBUG` | debug | General debug mode |
| `DH_DEBUG_CDC_FLASH` | debug | CDC serial command to trigger bootloader |
| `ENABLE_USER_OVERRIDES` | both | User-specific device override support |

## Architecture

**Dual-Core**: Core 0 handles USB device (to computers) and UART TX; Core 1 handles USB host (input devices), UART RX, and LED control.

**Key components**:
- `hid_parser.c/h`: Parse HID descriptors
- `hid_report.c/h`: Process HID reports
- `mouse.c/h`: Absolute coordinates, screen edge switching
- `keyboard.c/h`: Key tracking, LED state
- `handlers.c/h`: Hotkey detection
- `protocol.c`, `uart.c`: Inter-board communication
- `ramdisk.c`: Config mode FAT filesystem

**State**: `device_t global_state` in `src/include/structs.h`

## Configuration

**Compile-time**: Edit `src/include/user_config.h` for defaults (hotkeys, mouse speed, OS type, screensaver, etc.)

**Runtime**: `Left Ctrl + Right Shift + C + O` → opens "DESKHOP" drive → open `config.htm` in Chrome → configure → exit. See [README.md](README.md#web-configuration-mode) for details.

## Version & Flashing

**Version**: Defined in `CMakeLists.txt` as `VERSION_MAJOR`/`VERSION_MINOR`. Internal format: `MAJOR * 1000 + MINOR + 100`.

**Flashing**: See [README.md](README.md#upgrading-firmware) for upgrade methods (config mode or ROM bootloader).

## Development Patterns

**Hotkeys**: Add to `handlers.c` detection logic, implement `*_hotkey_handler()` function, add to detection array.

**Mouse behavior**: Modify `mouse.c` (handles coordinate accumulation, boundary detection, screen switching).

**Config options**: Add to `device_t` in `structs.h`, set default in `defaults.c`, update `webconfig/templates/`, rebuild with `./render.py`.
