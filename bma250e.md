# Fixing screen auto-rotate
<small>1. Do copy __60-sensor.hwdb__ into system folder by same path
```bash
sudo cp device_irbis_tw48/usr/lib/udev/hwdb.d/60-sensor.hwdb /usr/lib/udev/hwdb.d/
```
2. Do update hwdb and reload rules
```bash
sudo systemd-hwdb update && sudo udevadm control --reload-rules && sudo udevadm trigger
```
3. Do reboot tablet
```bash
sudo reboot
```
4. Do check screen auto-rotate
</small>
