#!/bin/bash

# Script to monitor DeskHop USB CDC serial output
# Can be called by flash.sh or run standalone

set -e

# Configuration
BAUD_RATE=115200
TIMEOUT=2

# Print usage
usage() {
  echo "Usage: $0 [OPTIONS]"
  echo ""
  echo "Monitor DeskHop USB CDC serial output"
  echo ""
  echo "Options:"
  echo "  -d, --device DEVICE    Serial device path (auto-detected if not specified)"
  echo "  -b, --baud RATE        Baud rate (default: 115200)"
  echo "  -t, --timeout SEC      Device detection timeout in seconds (default: 2)"
  echo "  -h, --help             Show this help message"
  echo ""
  echo "Examples:"
  echo "  $0                                    # Auto-detect device"
  echo "  $0 -d /dev/tty.usbmodem1234          # Use specific device"
  echo "  $0 -b 9600                            # Use different baud rate"
  exit 0
}

# Parse arguments
DEVICE=""
while [[ $# -gt 0 ]]; do
  case $1 in
    -d|--device)
      DEVICE="$2"
      shift 2
      ;;
    -b|--baud)
      BAUD_RATE="$2"
      shift 2
      ;;
    -t|--timeout)
      TIMEOUT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      ;;
    *)
      echo "Unknown option: $1"
      usage
      ;;
  esac
done

# Auto-detect device if not specified
if [ -z "$DEVICE" ]; then
  echo "Searching for DeskHop CDC device..."
  elapsed=0
  while [ $elapsed -lt $TIMEOUT ]; do
    for device in /dev/tty.usbmodem*; do
      if [ -e "$device" ]; then
        DEVICE="$device"
        echo "Found device: $DEVICE"
        break 2
      fi
    done
    sleep 0.5
    elapsed=$((elapsed + 1))
  done

  if [ -z "$DEVICE" ]; then
    echo "Error: No DeskHop CDC device found after ${TIMEOUT}s"
    echo ""
    echo "Please ensure:"
    echo "  - DeskHop is connected via USB"
    echo "  - Firmware is running (not in bootloader mode)"
    echo "  - USB CDC is enabled in firmware (DH_DEBUG build)"
    exit 1
  fi
fi

# Verify device exists
if [ ! -e "$DEVICE" ]; then
  echo "Error: Device not found: $DEVICE"
  exit 1
fi

echo "Monitoring $DEVICE at $BAUD_RATE baud"
echo "Press Ctrl+C to exit"
echo "----------------------------------------"
echo ""

# Configure serial port settings
stty -f "$DEVICE" $BAUD_RATE cs8 -cstopb -parenb raw

# Simply cat the device - read-only, no terminal takeover
cat "$DEVICE"
