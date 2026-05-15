# Memory

## Goal
Monitor PC metrics (CPU, RAM, GPU, temps, disk, VRAM) on PandaTouch (ESP32-S3, LVGL 9.x) with WiFi config and clear UI.

## Build
```
.\.venv\Scripts\pio run -e pandatouch
```
(or just `pio run -e pandatouch` if .venv is active)

## Architecture
ESP32 reads LibreHardwareMonitor directly via HTTP (`PC_IP:8085/data.json`).
Python script (`pc_monitor.py`) is **optional** — only for FPS capture via PresentMon 2.x.
PresentMon v1.9.1 had no downloadable binaries. Now using v2.4.1 with `--output_stdout --terminate_on_proc_exit` (no `--simple`/`--capture_all` flags in v2.x). CSV header parsed dynamically for `MsBetweenPresents` column.

## Key Decisions
- `server.onRequestBody()` global instead of 5th param body handler in `server.on()` for POST /api/metrics (empty handler in `on()` caused HTTP 501).
- Temperatures on Windows via LibreHardwareMonitor HTTP (localhost:8085/data.json) first, fallbacks: wmic (MSAcpi) for CPU/MoBo, nvidia-smi for GPU.
- Python sends flat JSON payload for ArduinoJson compatibility.
- UI redesigned: 4 cards (no duplicates), large temp top-right (font 28, color-coded), VRAM bar in GPU card.
- Config button moved to top-right header row (same level as IP label).
- All cards same height 195px; VRAM bar at bottom-50, main bars at bottom-28.
- PC IP stored in Preferences, configurable via WiFi config screen.
- JSON parsing uses PSRAM allocator (`heap_caps_malloc` with `MALLOC_CAP_SPIRAM`) via custom `PSRAMAlloc` class, so large LHM trees don't exhaust DRAM.

## Critical Context
- `#define WEBSERVER_H` in webserver.cpp before `#include <ESPAsyncWebServer.h>` avoids HTTP method declaration conflicts.
- LibreHardwareMonitor needs Web Server enabled (Settings → Web Server, port 8085).
- `psutil.sensors_temperatures()` doesn't work on Windows → use LHM/WMI/nvidia-smi.
- Python interval ~1s, request timeout 5s.
- WiFi config uses 3 static globals (`g_wifi_ssid_ta`, `g_wifi_pass_ta`, `g_wifi_kb`) instead of `lv_obj_get_user_data` to avoid keyboard focus conflicts.
- PC IP must be static (or DHCP reservation) for reliable LHM polling.
- PresentMon v2.4.1 URL: `https://github.com/GameTechDev/PresentMon/releases/download/v2.4.1/PresentMon-2.4.1-x64.exe` (v1.9.1 had no assets). Flags: `--output_stdout --terminate_on_proc_exit`.
- CSV header `MsBetweenPresents` column index found dynamically via `csv.reader` to handle column order changes.

## File Layout
- `src/metrics.h`: PCMetrics struct (CPU, RAM, GPU, temps, VRAM, disk).
- `src/webserver.cpp`: `init_webserver()` / `check_wifi_status()`, POST handler for /api/metrics via `server.onRequestBody()`.
- `src/ui_monitor.cpp`: UI — 4 cards (195px each), large temp top-right with font 28 + thermal color, VRAM bar + percentage, config button top-right.
- `src/ui_wifi_config.cpp`: WiFi config with 3 static globals + PC IP textarea.
- `src/monitor_app.cpp`: loop calls `check_wifi_status()` + `fetch_lhm_data()` every 1s + `update_monitor_ui()`.
- `src/storage.h/cpp`: Preferences storage — WiFi creds, brightness, bg color, PC IP (`g_pc_ip`).
- `pc_monitor.py`: optional — sends FPS to ESP32 via POST /api/metrics using PresentMon 2.4.1.
- `include/lv_conf.h`: enables `LV_FONT_MONTSERRAT_28`.
