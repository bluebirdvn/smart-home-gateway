#!/bin/sh

sleep 10
if ! ip link show wlan0 | grep -q "UP"; then
    echo "Bring up wlan0 interface.."
    ip link set wlan0 up
    sleep 2
fi

if ! systemctl is-active --quiet wpa_supplicant@wlan0.service; then
    echo "Starting wpa_supplicant.service..."
    systemctl restart wpa_supplicant@wlan0.service
    sleep 5
fi

if ! ip addr show wlan0 | grep -q "inet"; then
    echo "No IP address. Restarting dhcp client..."
    /sbin/dhcpcd -n wlan0
    sleep 5
fi

if ip addr show wlan0 | grep -q "inet"; then
    echo "get addr successfully"
    exit 0
else 
    echo "wifi connection failed"
    ip link set wlan0 down
    sleep 2
    ip link set wlan0 up
    sleep 2
    systemctl restart wpa_supplicant@wlan0.service
    sleep 5
    /sbin/dhcpcd -n wlan0
    exit 1
fi