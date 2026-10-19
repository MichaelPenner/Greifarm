#!/usr/bin/bashio
echo "Starting Claw Machine Backend..."

# Read configuration from Home Assistant Add-on UI
export CLAW_ESP_HOST=$(bashio::config 'esp_host')
export CLAW_STEPS_PER_MM_XY=$(bashio::config 'steps_per_mm_xy')
export CLAW_STEPS_PER_MM_Z=$(bashio::config 'steps_per_mm_z')
export CLAW_MAX_SPEED_XY=$(bashio::config 'max_speed_xy')
export CLAW_MAX_SPEED_Z=$(bashio::config 'max_speed_z')
export CLAW_MAX_MM_X=$(bashio::config 'max_mm_x')
export CLAW_MAX_MM_Y=$(bashio::config 'max_mm_y')
export CLAW_MAX_MM_Z=$(bashio::config 'max_mm_z')
echo "Using ESP32 IP Address: ${CLAW_ESP_HOST}"

cd /app/backend
python3 main.py
