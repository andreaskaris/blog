In order to protect your Lenovo battery, you can set charge start and end thresholds.
According to [anecdotal evidence](https://linrunner.de/tlp/faq/battery.html#battery-care), good values are 40% to 50%.

```
cat <<'EOF' > /etc/systemd/system/battery-max.service
[Unit]
Description=Set the battery charge threshold
After=multi-user.target

StartLimitBurst=0
[Service]
Type=oneshot
Restart=on-failure

ExecStart=/bin/bash -c 'echo 50 > /sys/class/power_supply/BAT0/charge_control_end_threshold; echo 40 > /sys/class/power_supply/BAT0/charge_start_threshold'
[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable --now battery-max.service
```

