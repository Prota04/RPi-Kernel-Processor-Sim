#!/bin/bash
set -e

PROJECT_DIR="$HOME/Projekti/RTOS"
CPU_EMULATOR_DRIVER_DIR="$PROJECT_DIR/cpu_driver"
TRACE_COLLECTOR_DRIVER_DIR="$PROJECT_DIR/trace_collector_driver"
USERSPACE_DIR="$PROJECT_DIR/userspace"

CLOCK_SPEED=""

# Parse clock speed if provided
if [ $# -ge 1 ] && [[ "$1" =~ ^[0-9]+$ ]]; then
    CLOCK_SPEED="$1"
    shift
fi

echo "Removing old modules if loaded..."
sudo rmmod cpu_emulator_driver 2>/dev/null || true
sudo rmmod trace_collector_driver 2>/dev/null || true

# Loading CPU Emulator Driver
if [ -n "$CLOCK_SPEED" ]; then
    echo "Loading CPU driver with clock_speed set to ${CLOCK_SPEED}ms"
    sudo insmod "$CPU_EMULATOR_DRIVER_DIR/cpu_emulator_driver.ko" clock_speed="$CLOCK_SPEED"
else
    echo "Loading CPU driver with default clock_speed (500ms)"
    sudo insmod "$CPU_EMULATOR_DRIVER_DIR/cpu_emulator_driver.ko"
fi

echo "Loading trace collector driver..."
sudo insmod "$TRACE_COLLECTOR_DRIVER_DIR/trace_collector_driver.ko"

echo "Running userspace program..."
cd "$USERSPACE_DIR"
./bin/Release/main "$@"