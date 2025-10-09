#!/bin/bash

set -e # Exit on any error

# Print usage information
usage() {
  echo "Usage: $0 [-d|--debug] [-f [N]|--flash[=N]] [-n|--no-build] [--docker] [-p|--preset PRESET]"
  echo "  -d, --debug         Build in debug mode"
  echo "  -f [N], --flash[=N] Flash N devices after building (default: 1, max: 2)"
  echo "                      Examples: -f (flash 1), -f 2 (flash 2), --flash=2"
  echo "  -n, --no-build      Don't build first"
  echo "  --docker            Build with Docker (default: local cmake build)"
  echo "  -p, --preset PRESET Use specific CMake preset (overrides -d and --docker)"
  echo ""
  echo "Available presets (use 'cmake --list-presets' for full list):"
  echo "  mac-release, mac-debug, docker-release, docker-debug"
  exit 1
}

flash_device() {
  local device_num=$1
  local total_devices=$2
  local build_dir=$3
  RPI_VOLUME="/Volumes/RPI-RP2"

  echo -n "Waiting for device $device_num of $total_devices..."
  # Wait for the RPI volume to appear and be non-empty
  while [ ! -d "$RPI_VOLUME" ] || [ -z "$(ls -A $RPI_VOLUME)" ]; do
    echo -n "."
    sleep 1
  done
  echo

  echo "Found RPI-RP2 volume, copying UF2 file (device $device_num of $total_devices)..."
  
  cp "$build_dir/deskhop.uf2" "$RPI_VOLUME/" || echo "cp failed, continuing..."

  # Wait for the volume to disappear (which happens after flashing)
  echo -n "Waiting for flashing to complete..."
  while [ -d "$RPI_VOLUME" ]; do
    echo -n "."
    sleep 1
  done
  echo

  echo "Flash complete for device $device_num!"
}

bump_version() {
  VERSION_FILE=".version"
  CMAKE_FILE="CMakeLists.txt"
  
  # Read current minor version from .version file, or from CMakeLists.txt if .version doesn't exist
  if [ -f "$VERSION_FILE" ]; then
    CURRENT_MINOR=$(cat "$VERSION_FILE")
  else
    # Initialize from CMakeLists.txt
    CURRENT_MINOR=$(grep "set(VERSION_MINOR" "$CMAKE_FILE" | sed -E 's/.*set\(VERSION_MINOR ([0-9]+)\).*/\1/')
  fi
  
  # Increment it
  NEW_MINOR=$((CURRENT_MINOR + 1))
  
  echo "Bumping version: 0.$CURRENT_MINOR -> 0.$NEW_MINOR"
  
  # Save to .version file
  echo "$NEW_MINOR" > "$VERSION_FILE"
}

# Initialize flags
DEBUG=0
FLASH_COUNT=0
BUILD=1
USE_DOCKER=0
PRESET=""

# Parse command line arguments using getopt
TEMP=$(getopt -o df::nhp: -l debug,flash::,no-build,docker,help,preset: -n "$0" -- "$@")
if [ $? != 0 ]; then
  echo "Error parsing options" >&2
  usage
fi

eval set -- "$TEMP"

while true; do
  case "$1" in
    -d|--debug)
      DEBUG=1
      shift
      ;;
    -f|--flash)
      if [ -n "$2" ]; then
        FLASH_COUNT="$2"
        shift 2
      else
        FLASH_COUNT=1
        shift
      fi
      ;;
    -n|--no-build)
      BUILD=0
      shift
      ;;
    --docker)
      USE_DOCKER=1
      shift
      ;;
    -p|--preset)
      PRESET="$2"
      shift 2
      ;;
    -h|--help)
      usage
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "Internal error!"
      exit 1
      ;;
  esac
done

# Validate flash count
if [ $FLASH_COUNT -gt 2 ]; then
  echo "Error: Maximum 2 devices can be flashed"
  exit 1
fi

# Determine preset to use
if [ -z "$PRESET" ]; then
  # Auto-select preset based on flags
  if [ $USE_DOCKER -eq 1 ]; then
    if [ $DEBUG -eq 1 ]; then
      PRESET="docker-debug"
    else
      PRESET="docker-release"
    fi
  else
    if [ $DEBUG -eq 1 ]; then
      PRESET="mac-debug"
    else
      PRESET="mac-release"
    fi
  fi
fi

# Determine build directory from preset
if [[ "$PRESET" == docker-* ]]; then
  BUILD_DIR="build"
else
  BUILD_DIR="build/mac"
fi

# Run build
if [ $BUILD -eq 1 ]; then
  bump_version
  
  # Get the updated VERSION_MINOR after bump
  VERSION_FILE=".version"
  VERSION_MINOR=$(cat "$VERSION_FILE")
  
  if [[ "$PRESET" == docker-* ]]; then
    # Docker build using preset
    echo "Building with Docker using preset: ${PRESET}..."
    docker-compose -f misc/docker.yml run --rm build_container sh -c "cmake --preset=${PRESET} -DVERSION_MINOR=${VERSION_MINOR} && cmake --build --preset=${PRESET}"
  else
    # Local build using preset
    echo "Building locally using preset: ${PRESET}..."
    cmake --preset=${PRESET} -DVERSION_MINOR=${VERSION_MINOR}
    cmake --build --preset=${PRESET}
  fi
fi

# Flash if requested
if [ $FLASH_COUNT -gt 0 ]; then
  for device in $(seq 1 $FLASH_COUNT); do
    flash_device "$device" "$FLASH_COUNT" "$BUILD_DIR"
  done
  echo "All $FLASH_COUNT device(s) flashed successfully!"
else
  echo "Build complete (in ${BUILD_DIR}). Use -f or --flash option to flash devices."
fi

