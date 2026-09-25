mkdir -p /root/www/Greifarm
cd /root/www/Greifarm
wget http://192.168.178.36:8000/greifarm_upload.zip
unzip -o greifarm_upload.zip
bash deploy/setup_pi.sh
