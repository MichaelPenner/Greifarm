#!/usr/bin/bashio
echo "Starting Claw Machine Backend..."

# Read configuration from Home Assistant Add-on UI
export CLAW_ESP_HOST=$(bashio::config 'esp_host')
echo "Using ESP32 IP Address: ${CLAW_ESP_HOST}"

cd /app/backend
python3 main.py
