#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "typer>=0.12.0",
#     "rich>=13.0.0",
#     "pyserial>=3.5",
# ]
# ///

"""
DeskHop - Flash and monitor tool
"""

import sys
import threading
import time
from pathlib import Path
from typing import Optional

import typer
from rich.console import Console
from rich.progress import Progress, SpinnerColumn, TextColumn
import serial.tools.list_ports

app = typer.Typer(help="DeskHop firmware management tool", no_args_is_help=True)
out = Console()


# Get project root (parent of misc/)
PROJECT_ROOT = Path(__file__).parent.parent

# Constants
RPI_VOLUME = Path("/Volumes/RPI-RP2")
CDC_TIMEOUT = 5
VOLUME_WAIT_TIMEOUT = 30
FLASH_WAIT_TIMEOUT = 15
DEFAULT_BAUD_RATE = 115200
DEVICE_DETECT_TIMEOUT = 2


def find_cdc_device() -> Optional[Path]:
    """Find a DeskHop CDC device by checking product name"""
    DESKHOP_PRODUCT = "DeskHop Switch"

    for port in serial.tools.list_ports.comports():
        if port.product and DESKHOP_PRODUCT in port.product:
            return Path(port.device)
    return None


def wait_for_volume(timeout: int = VOLUME_WAIT_TIMEOUT) -> bool:
    """Wait for RPI-RP2 volume to appear"""
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=out
    ) as progress:
        task = progress.add_task("Waiting for device...", total=None)

        elapsed = 0
        while elapsed < timeout:
            if RPI_VOLUME.exists() and list(RPI_VOLUME.iterdir()):
                return True
            time.sleep(1)
            elapsed += 1

        return False


def wait_for_volume_disappear(timeout: int = FLASH_WAIT_TIMEOUT) -> bool:
    """Wait for RPI-RP2 volume to disappear after flashing"""
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=out
    ) as progress:
        task = progress.add_task("Waiting for flashing to complete...", total=None)

        elapsed = 0
        while elapsed < timeout:
            if not RPI_VOLUME.exists():
                return True
            time.sleep(1)
            elapsed += 1

        return False


def trigger_bootloader(device: Path) -> bool:
    """Try to trigger bootloader mode via CDC"""
    try:
        with open(device, 'wb') as f:
            f.write(b'flash')
        time.sleep(1)
        return True
    except Exception:
        return False


@app.command()
def flash(
    firmware: Optional[Path] = typer.Argument(None, help="Path to .uf2 firmware file"),
    debug: bool = typer.Option(False, "--debug", "-d", help="Use debug build"),
    preset: Optional[str] = typer.Option(None, "--preset", "-p", help="Use specific CMake preset")
):
    """Flash a DeskHop device"""

    # Determine what was provided and find the UF2 file
    if firmware is None:
        if preset is None:
            preset = "debug" if debug else "release"
        uf2_file = PROJECT_ROOT / f"build/{preset}/deskhop.uf2"
    else:
        # Direct path to UF2 file
        uf2_file = firmware
        if not uf2_file.suffix == ".uf2":
            out.print(f"[red]Error:[/red] Firmware must be a .uf2 file, got: {uf2_file}")
            raise typer.Exit(1)

    # Verify UF2 file exists
    if not uf2_file.exists():
        out.print(f"[red]Error:[/red] UF2 file not found at: {uf2_file}")
        out.print("\nPlease build the firmware first or specify the correct path.")
        raise typer.Exit(1)

    out.print(f"[blue]Using firmware:[/blue] {uf2_file}")
    out.print()

    # Try to find and trigger CDC bootloader
    cdc_device = find_cdc_device()
    if cdc_device:
        out.print(f"Found usbmodem device at {cdc_device}")
        out.print("Attempting to trigger bootloader mode via CDC...")
        if trigger_bootloader(cdc_device):
            out.print("Command sent!")
            out.print("If you don't see a flash LED, then put your device into bootloader mode manually:")
        else:
            out.print("CDC command failed")
            out.print("Please put your device into bootloader mode manually:")
    else:
        out.print("No DeskHop CDC device found.")
        out.print("Please put your device into bootloader mode manually:")

    out.print("  - Hold the BOOTSEL button while plugging in the USB cable")
    out.print("  - Or hold BOOTSEL and press the RESET button")
    out.print()

    # Wait for volume
    if not wait_for_volume():
        out.print(f"[red]Error:[/red] Timeout waiting for RPI-RP2 volume after {VOLUME_WAIT_TIMEOUT}s")
        out.print("Please ensure your device is in bootloader mode and try again.")
        raise typer.Exit(1)

    out.print("[green]Found RPI-RP2 volume![/green]")
    out.print("Copying UF2 file...")

    # Copy file
    try:
        import shutil
        shutil.copy(uf2_file, RPI_VOLUME / "deskhop.uf2")
    except Exception as e:
        out.print(f"[yellow]Warning:[/yellow] Copy failed ({e}), continuing...")

    # Wait for flash to complete
    if not wait_for_volume_disappear():
        out.print(f"[yellow]Warning:[/yellow] Timeout waiting for flash completion after {FLASH_WAIT_TIMEOUT}s")
        out.print("The device may still be flashing. Check the device status.")
    else:
        out.print("[green]✓ Flash complete![/green]")


def _wait_for_device(device: Optional[Path], timeout: Optional[int]) -> Optional[Path]:
    """Wait for a DeskHop CDC device to appear, return its path or None.
    If timeout is None, wait indefinitely."""
    start = time.time()
    while timeout is None or time.time() - start < timeout:
        if device is not None:
            if device.exists():
                return device
        else:
            found = find_cdc_device()
            if found:
                return found
        time.sleep(0.5)
    return None


@app.command()
def console(
    device: Optional[Path] = typer.Option(None, "--device", "-d", help="Serial device path"),
    baud: int = typer.Option(DEFAULT_BAUD_RATE, "--baud", "-b", help="Baud rate"),
    timeout: int = typer.Option(30, "--timeout", "-t", help="Device detection timeout in seconds"),
    reconnect: bool = typer.Option(True, "--reconnect/--no-reconnect", help="Reconnect after disconnect"),
):
    """Interactive serial console for DeskHop (read/write)"""
    import serial

    out.print("Press Ctrl+C to exit")
    out.print("----------------------------------------")

    first_connect = True
    while True:
        out.print("Waiting for DeskHop CDC device...")
        # Use timeout only on first connect; reconnects wait indefinitely
        wait_timeout = timeout if first_connect else None
        found = _wait_for_device(device, wait_timeout)

        if found is None:
            out.print(f"[red]Error:[/red] No DeskHop CDC device found after {timeout}s")
            out.print("Please ensure:")
            out.print("  - DeskHop is connected via USB")
            out.print("  - Firmware is running (not in bootloader mode)")
            out.print("  - USB CDC is enabled in firmware (DH_DEBUG build)")
            raise typer.Exit(1)

        first_connect = False

        out.print(f"[green]Connected:[/green] {found} at {baud} baud")

        try:
            with serial.Serial(str(found), baud, timeout=1) as ser:
                ser.write(b'version\n')

                def stdin_to_serial():
                    try:
                        for line in sys.stdin:
                            ser.write(line.encode('utf-8'))
                    except Exception:
                        pass

                t = threading.Thread(target=stdin_to_serial, daemon=True)
                t.start()

                while True:
                    data = ser.read(1024)
                    if data:
                        sys.stdout.write(data.decode('utf-8', errors='replace'))
                        sys.stdout.flush()
        except serial.SerialException as e:
            out.print(f"\n[yellow]Disconnected:[/yellow] {e}")
        except KeyboardInterrupt:
            raise typer.Exit(0)
        except Exception as e:
            out.print(f"\n[red]Error:[/red] {e}")
            raise typer.Exit(1)

        if not reconnect:
            raise typer.Exit(0)

        out.print("Reconnecting...")


if __name__ == "__main__":
    app()
