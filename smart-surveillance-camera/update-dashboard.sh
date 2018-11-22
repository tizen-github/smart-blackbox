cd dashboard

sdb root on;
sdb shell 'mount -o remount,rw /';

sdb shell 'systemctl stop iot-dashboard'

sdb shell 'rm -rf /opt/home/dashboard'
sdb shell 'mkdir -p /opt/home/dashboard/public/js';
sdb shell 'mkdir -p /opt/home/dashboard/public/css';

sdb push server.js           /opt/home/dashboard;
sdb push default.gif          /opt/home/dashboard;
sdb push public/index.html    /opt/home/dashboard/public/index.html;
sdb push public/test.html     /opt/home/dashboard/public/test.html;
sdb push public/js/app.js     /opt/home/dashboard/public/js/app.js;
sdb push public/css/style.css /opt/home/dashboard/public/css/style.css;

sdb shell 'cd /opt/home/dashboard; find .';

# sdb shell 'cd /opt/home/dashboard; iotjs server.js'

sdb shell 'systemctl start iot-dashboard'
sdb shell 'systemctl status iot-dashboard'
