sdb root on
sdb shell 'mount -o remount,rw /'

sdb shell 'app_launcher -t iot-vision-camera'

sdb push ~/GBS-ROOT/local/BUILD-ROOTS/scratch.armv7l.0/home/abuild/rpmbuild/RPMS/armv7l/iot-vision-camera-0.0.1-1.armv7l.rpm /tmp
sdb shell 'cd /tmp; rpm -ivh --force iot-vision-camera-0.0.1-1.armv7l.rpm'

sdb shell 'app_launcher -s iot-vision-camera'

sdb shell 'systemctl status iot-dashboard'
