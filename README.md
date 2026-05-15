# PandaTouch PC Monitor

Monitor your PC's vital metrics in real-time on a **BigTreeTech PandaTouch** (ESP32-S3) display. Shows CPU, RAM, GPU, temperatures, disk, VRAM, and FPS — all read directly from your PC via WiFi.

## Features

- **CPU**: Usage, frequency, package power, fan RPM
- **RAM**: Used/total GB, DIMM temperatures (min/max)
- **GPU**: Usage, core/memory clocks, temperatures (core/hotspot/mem), VRAM bar with percentage, fan RPM, package power
- **System**: Motherboard, PCH, VRM, SSD temperatures — fan ranges, pump speed, total system power (CPU+GPU)
- **FPS**: Real-time frames per second from any 3D application (via optional PresentMon)
- **Large temperature display**: Color-coded thermal readout in the top-right corner
- **WiFi configuration**: Configure WiFi and PC IP directly from the touchscreen

## How It Works

| Metric | Source | Method |
|--------|--------|--------|
| CPU/GPU temps, load, clocks, power | LibreHardwareMonitor | HTTP `PC_IP:8085/data.json` (direct from ESP32) |
| RAM, VRAM, disk | LibreHardwareMonitor | Same JSON payload |
| FPS | PresentMon 2.x (optional) | Python script sends POST to ESP32 |

### Requirements

- **PandaTouch** (ESP32-S3 with 16MB flash, Octal PSRAM, LVGL display)
- **PC with LibreHardwareMonitor** (enable Web Server on port 8085 in Settings)
- **Optional**: Python 3.10+ for FPS capture via PresentMon

---

## Installation

### Option A: Web Installer (No-Code, Easy)

The fastest way. You only need a **Chrome-based browser**:

1. Download the latest `pandatouch_pcmonitor_vX.Y.Z_factory.bin` from the [Releases](https://github.com/Disttrack/PandaTouch_PC_Monitor/releases) page.
2. Go to [ESP Web Tools](https://web.esphome.io/).
3. Connect your PandaTouch via USB-C and click **Connect**.
4. Click **Install** and select the `.bin` file.
5. Wait for the process to finish. Your device will reboot.

> [!IMPORTANT]
> The first flash MUST be `factory.bin` via USB. This writes the bootloader, partition table, and firmware together.

### Option B: PlatformIO (Advanced)

For developers who want to modify the code:

1. Clone this repository:

   ```
   git clone https://github.com/Disttrack/PandaTouch_PC_Monitor.git
   cd PandaTouch_PC_Monitor
   ```

2. Create a Python virtual environment and install PlatformIO:

   ```
   python -m venv .venv
   .venv\Scripts\Activate.ps1
   pip install platformio
   ```

3. Connect your PandaTouch via USB-C, then flash:

   ```
   pio run -e pandatouch -t upload
   ```

> [!NOTE]
> The first installation requires a USB cable.

---

## Configuration

After installation, the PandaTouch will show a WiFi configuration screen:

1. **Connect to WiFi**: The device scans for networks. Select yours and enter the password.
2. **Set PC IP**: Enter the IP address of the PC running LibreHardwareMonitor.
3. **Save**: The device reboots and starts displaying metrics.

### LibreHardwareMonitor Setup

1. Download and run [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor).
2. Go to **Settings → Web Server**.
3. Enable the web server on **port 8085**.
4. Ensure your PC has a **static IP** or a DHCP reservation so the PandaTouch can always find it.

### Optional: FPS Capture

For real-time FPS from games and 3D applications:

1. Install Python 3.10+ on your PC.
2. Install the required package:

   ```
   pip install requests
   ```

3. Run the FPS sender:

   ```
   python pc_monitor.py --host 192.168.X.X
   ```

   (Replace with your PandaTouch IP address)

The script automatically downloads PresentMon 2.4.1 on first run. It sends FPS data to the ESP32 every second.



## Troubleshooting

| Problem | Solution |
|---------|----------|
| Screen flickering | Low battery — connect the device to its dock or USB charger. |
| No data on screen | Verify LibreHardwareMonitor is running on your PC with Web Server enabled. |
| `Connecting...` forever | Check that your PC IP is correct in the WiFi config screen. |
| FPS shows `--` | Ensure `pc_monitor.py` is running on your PC and the game is active. |
| PresentMon download fails | Download `PresentMon-2.4.1-x64.exe` manually from [releases](https://github.com/GameTechDev/PresentMon/releases/tag/v2.4.1) and place it next to `pc_monitor.py`. |

---

## License

MIT

---

## Acknowledgments

- [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) for system telemetry
- [PresentMon](https://github.com/GameTechDev/PresentMon) by Intel for frame capture
- [PandaTouch / BigTreeTech](https://github.com/bigtreetech/PandaTouch) for the hardware
- [PandaTouch StreamDeck](https://github.com/Disttrack/PandaTouch_streamDeck) — the sibling project that inspired this one
