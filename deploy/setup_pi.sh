#!/bin/bash
set -e

echo "🎪 Claw Machine - Raspberry Pi Setup Script"
echo "=========================================="

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo ./setup_pi.sh)"
  exit
fi

echo "[1/4] Installing system dependencies..."
apt-get update
apt-get install -y python3-pip python3-venv nginx sqlite3 libsqlite3-dev

echo "[2/4] Setting up Python virtual environment..."
PROJECT_DIR="/home/pi/claw-machine"
if [ ! -d "$PROJECT_DIR" ]; then
    echo "Error: Please copy the claw-machine folder to /home/pi/ first."
    exit 1
fi

cd "$PROJECT_DIR/backend"
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

echo "[3/4] Configuring systemd service..."
cp ../deploy/claw-machine.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable claw-machine.service
systemctl start claw-machine.service

echo "[4/4] Configuring Nginx..."
cp ../deploy/nginx.conf /etc/nginx/sites-available/claw-machine
ln -sf /etc/nginx/sites-available/claw-machine /etc/nginx/sites-enabled/
rm -f /etc/nginx/sites-enabled/default
systemctl restart nginx

echo "✅ Setup Complete!"
echo "Backend should be running on port 8000 internally."
echo "Nginx should be serving the app on port 80."
echo "Test by opening http://<raspberry-pi-ip>/ in your browser."
