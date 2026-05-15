#!/usr/bin/env python3
"""
PandaTouch PC Monitor — sends live system metrics to the ESP32 dashboard.
"""

import argparse
import json
import time
import requests
import psutil

try:
    import gputil
    HAS_GPUTIL = True
except ImportError:
    HAS_GPUTIL = False

DEFAULT_HOST = "192.168.20.240"
DEFAULT_INTERVAL = 1.0


def get_gpu_percent():
    if not HAS_GPUTIL:
        return 0.0
    try:
        gpus = gputil.getGPUs()
        return gpus[0].load * 100.0 if gpus else 0.0
    except Exception:
        return 0.0


def get_gpu_temp():
    if not HAS_GPUTIL:
        return 0.0
    try:
        gpus = gputil.getGPUs()
        return gpus[0].temperature if gpus else 0.0
    except Exception:
        return 0.0


def get_temps():
    temps = {"cpu": 0.0, "gpu": 0.0, "ssd": 0.0, "mobo": 0.0}
    if hasattr(psutil, "sensors_temperatures"):
        all_temps = psutil.sensors_temperatures()
        for name, entries in all_temps.items():
            if not entries:
                continue
            val = entries[0].current
            name_lower = name.lower()
            if "core" in name_lower or "cpu" in name_lower or "k10temp" in name_lower or "coretemp" in name_lower:
                temps["cpu"] = val
            elif "gpu" in name_lower or "nvidia" in name_lower or "radeo" in name_lower:
                temps["gpu"] = val
            elif "nvme" in name_lower or "ssd" in name_lower or "disk" in name_lower:
                temps["ssd"] = val
            elif "motherboard" in name_lower or "acpitz" in name_lower or "pch" in name_lower:
                temps["mobo"] = val
    if temps["gpu"] == 0.0:
        temps["gpu"] = get_gpu_temp()
    return temps


def get_cpu_freq():
    try:
        freq = psutil.cpu_freq()
        return freq.current if freq else 0.0
    except Exception:
        return 0.0


def send_metrics(host, interval):
    url = f"http://{host}/api/metrics"
    print(f"Sending metrics to {url} every {interval}s (Ctrl+C to stop)")
    while True:
        try:
            cpu = psutil.cpu_percent(interval=0.5)
            ram = psutil.virtual_memory()
            temps = get_temps()

            payload = {
                "cpu": cpu,
                "ram": ram.percent,
                "gpu": get_gpu_percent(),
                "cpu_temp": temps["cpu"],
                "gpu_temp": temps["gpu"],
                "ssd_temp": temps["ssd"],
                "mobo_temp": temps["mobo"],
                "cpu_freq": get_cpu_freq(),
            }

            r = requests.post(url, json=payload, timeout=5)
            print(f"Sent: CPU={cpu:.0f}% RAM={ram.percent:.0f}% GPU={payload['gpu']:.0f}% | "
                  f"Temps CPU={temps['cpu']:.0f}C GPU={temps['gpu']:.0f}C "
                  f"SSD={temps['ssd']:.0f}C MoBo={temps['mobo']:.0f}C "
                  f"Freq={payload['cpu_freq']:.0f}MHz | HTTP {r.status_code}")
        except KeyboardInterrupt:
            print("\nStopped.")
            break
        except Exception as e:
            print(f"Error: {e}")

        time.sleep(interval)


def main():
    parser = argparse.ArgumentParser(description="PandaTouch PC Monitor sender")
    parser.add_argument("--host", default=DEFAULT_HOST, help="ESP32 IP address")
    parser.add_argument("--interval", type=float, default=DEFAULT_INTERVAL, help="Seconds between updates")
    args = parser.parse_args()
    send_metrics(args.host, args.interval)


if __name__ == "__main__":
    main()
