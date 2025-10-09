#!/bin/bash

set -e # Exit on any error

# Timeout settings (in seconds)
CDC_TIMEOUT=5
VOLUME_WAIT_TIMEOUT=30
FLASH_WAIT_TIMEOUT=15

# Print usage information
usage() {
  echo "Usage: $0 [OPTIONS] [BUILD_DIR]"
  echo ""
  echo "Flash a single device with the deskhop.uf2 firmware"
  echo ""
  echo "Options:"
  echo "  -m, --monitor      Start serial monitor after flashing"
  echo "  -h, --help         Show this help message"
  echo ""
  echo "Arguments:"
  echo "  BUILD_DIR          Path to build directory containing deskhop.uf2"
  echo "                     (default: build/mac)"
  echo ""
  echo "Examples:"
  echo "  $0                 # Flash using build/mac/deskhop.uf2"
  echo "  $0 -m              # Flash and start monitor"
  echo "  $0 build           # Flash using build/deskhop.uf2"
  echo "  $0 -m build/mac    # Flash build/mac and start monitor"
  exit 1
}

# Parse arguments
MONITOR_AFTER=false
BUILD_DIR=""

while [[ $# -gt 0 ]]; do
  case $1 in
    -m|--monitor)
      MONITOR_AFTER=true
      shift
      ;;
    -h|--help)
      usage
      ;;
    *)
      if [ -z "$BUILD_DIR" ]; then
        BUILD_DIR="$1"
      else
        echo "Error: Multiple build directories specified"
        usage
      fi
      shift
      ;;
  esac
done

BUILD_DIR="${BUILD_DIR:-build/mac}"
UF2_FILE="$BUILD_DIR/deskhop.uf2"
RPI_VOLUME="/Volumes/RPI-RP2"

# Verify the UF2 file exists
if [ ! -f "$UF2_FILE" ]; then
  echo "Error: UF2 file not found at: $UF2_FILE"
  echo ""
  echo "Please build the firmware first or specify the correct build directory."
  exit 1
fi

echo "Using firmware: $UF2_FILE"
echo ""

# Try to find DeskHop CDC device and trigger bootloader mode
CDC_DEVICE=""
for device in /dev/tty.usbmodem*; do
  if [ -e "$device" ]; then
    echo "Found usbmodem device at $device"
    CDC_DEVICE="$device"
    break
  fi
done || true

if [ ! -z "$CDC_DEVICE" ]; then
  echo "Attempting to trigger bootloader mode via CDC..."
  # Use gtimeout if available (from coreutils), otherwise use a simple approach
  if command -v gtimeout >/dev/null 2>&1; then
    if gtimeout $CDC_TIMEOUT bash -c "echo -n 'flash' > '$CDC_DEVICE' 2>/dev/null && sleep 1"; then
      echo "Command sent!"
      echo "If you don't see a flash LED, then put your device into bootloader mode manually:"
    else
      echo "CDC command timed out after ${CDC_TIMEOUT}s"
      echo "Please put your device into bootloader mode manually:"
    fi
  else
    # Fallback: try the command without timeout (it should be quick anyway)
    if echo -n 'flash' > "$CDC_DEVICE" 2>/dev/null && sleep 1; then
      echo "Command sent!"
      echo "If you don't see a flash LED, then put your device into bootloader mode manually:"
    else
      echo "CDC command failed"
      echo "Please put your device into bootloader mode manually:"
    fi
  fi
else
  echo "No DeskHop CDC device found."
  echo "Please put your device into bootloader mode manually:"
fi

echo "  - Hold the BOOTSEL button while plugging in the USB cable"
echo "  - Or hold BOOTSEL and press the RESET button"
echo ""

echo -n "Waiting for device..."
# Wait for the RPI volume to appear and be non-empty with timeout
elapsed=0
while [ ! -d "$RPI_VOLUME" ] || [ -z "$(ls -A $RPI_VOLUME 2>/dev/null)" ]; do
  if [ $elapsed -ge $VOLUME_WAIT_TIMEOUT ]; then
    echo
    echo "Error: Timeout waiting for RPI-RP2 volume after ${VOLUME_WAIT_TIMEOUT}s"
    echo "Please ensure your device is in bootloader mode and try again."
    exit 1
  fi
  echo -n "."
  sleep 1
  elapsed=$((elapsed + 1))
done
echo
echo "Found RPI-RP2 volume!"

echo "Copying UF2 file..."
# dd if="$UF2_FILE" of="$RPI_VOLUME/deskhop.uf2" bs=1m 2>/dev/null || echo "dd failed, continuing..."
cp "$UF2_FILE" "$RPI_VOLUME/deskhop.uf2" 2>/dev/null || echo "cp failed, continuing..."

# Wait for the volume to disappear (which happens after flashing) with timeout
echo -n "Waiting for flashing to complete..."
elapsed=0
while [ -d "$RPI_VOLUME" ]; do
  if [ $elapsed -ge $FLASH_WAIT_TIMEOUT ]; then
    echo
    echo "Warning: Timeout waiting for flash completion after ${FLASH_WAIT_TIMEOUT}s"
    echo "The device may still be flashing. Check the device status."
    break
  fi
  echo -n "."
  sleep 1
  elapsed=$((elapsed + 1))
done
echo

echo "✓ Flash complete!"

# Start monitor if requested
if [ "$MONITOR_AFTER" = true ]; then
  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  MONITOR_SCRIPT="$SCRIPT_DIR/monitor.sh"

  if [ -f "$MONITOR_SCRIPT" ]; then
    echo ""
    echo "Starting serial monitor..."
    sleep 2  # Give device time to enumerate
    exec "$MONITOR_SCRIPT"
  else
    echo ""
    echo "Warning: monitor.sh not found at $MONITOR_SCRIPT"
    echo "Skipping serial monitor."
  fi
fi

