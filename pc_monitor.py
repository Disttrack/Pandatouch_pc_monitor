#!/usr/bin/env python3
"""
PandaTouch PC Monitor — sends FPS to the ESP32 dashboard.
Uses PresentMon (ETW-based) to capture FPS from any 3D application.
"""

import argparse
import csv
import io
import os
import sys
import time
import requests
import subprocess
import urllib.request

DEFAULT_HOST = "192.168.20.240"
DEFAULT_INTERVAL = 1.0
PM_URL = "https://github.com/GameTechDev/PresentMon/releases/download/v2.4.1/PresentMon-2.4.1-x64.exe"
PM_EXE = "PresentMon-2.4.1-x64.exe"


def ensure_presentmon():
    """Download PresentMon if not already present."""
    local = os.path.join(os.path.dirname(__file__), PM_EXE)
    if os.path.exists(local):
        return local

    print(f"Downloading PresentMon from {PM_URL}...")
    try:
        urllib.request.urlretrieve(PM_URL, local)
        os.chmod(local, 0o755)
        print(f"Saved to {local}")
        return local
    except Exception as e:
        print(f"Download failed: {e}")
        return None


def send_fps(host, interval):
    local_pm = ensure_presentmon()
    if not local_pm:
        print("PresentMon not available. Cannot capture FPS.")
        return

    url = f"http://{host}/api/metrics"
    print(f"Sending FPS to {url} every {interval}s (Ctrl+C to stop)")
    print(f"Using PresentMon: {local_pm}")
    sys.stdout.flush()

    proc = subprocess.Popen(
        [local_pm, "--output_stdout", "--terminate_on_proc_exit"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        bufsize=1, universal_newlines=True
    )

    time.sleep(2)
    poll = proc.poll()
    if poll is not None:
        stderr = proc.stderr.read()
        print(f"PresentMon exited with code {poll}")
        if stderr:
            print(f"Stderr: {stderr}")
        return

    # Read CSV header to find MsBetweenPresents column
    header_line = None
    for line in proc.stdout:
        line = line.strip()
        if line:
            header_line = line
            break

    if not header_line:
        print("No CSV header from PresentMon")
        return

    columns = [c.strip() for c in header_line.split(",")]
    try:
        col_idx = columns.index("MsBetweenPresents")
    except ValueError:
        print(f"MsBetweenPresents not found in CSV columns: {columns}")
        return

    fps_values = []
    last_report = time.time()

    try:
        for line in proc.stdout:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            try:
                parts = next(csv.reader(io.StringIO(line)))
                if len(parts) > col_idx:
                    ms = float(parts[col_idx].strip())
                    if ms > 0 and ms < 1000:
                        fps_values.append(1000.0 / ms)
            except (ValueError, IndexError, StopIteration):
                pass

            now = time.time()
            if now - last_report >= interval:
                last_report = now
                avg_fps = sum(fps_values) / len(fps_values) if fps_values else 0.0
                fps_values = []

                payload = {"fps": round(avg_fps, 1)}
                try:
                    r = requests.post(url, json=payload, timeout=5)
                    print(f"FPS: {avg_fps:.0f} | HTTP {r.status_code}")
                except Exception as e:
                    print(f"POST error: {e}")
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()


def main():
    parser = argparse.ArgumentParser(description="PandaTouch FPS sender (PresentMon)")
    parser.add_argument("--host", default=DEFAULT_HOST, help="ESP32 IP address")
    parser.add_argument("--interval", type=float, default=DEFAULT_INTERVAL,
                        help="Seconds between FPS updates")
    args = parser.parse_args()
    send_fps(args.host, args.interval)


if __name__ == "__main__":
    main()
