# Changelog

## [v1.0.0] - 2026-05-15

### Initial Release

- **CPU Monitoring**: Usage, frequency, package power, fan RPM
- **RAM Monitoring**: Used/total GB, DIMM temperatures (min/max)
- **GPU Monitoring**: Usage, core/memory clocks, temperatures (core/hotspot/mem), VRAM bar with percentage, fan RPM, package power
- **System Monitoring**: Motherboard, PCH, VRM, SSD temperatures, fan ranges, pump speed, total system power (CPU+GPU)
- **FPS Tracking**: Real-time FPS from any 3D application via optional PresentMon 2.4.1
- **Large Temperature Display**: Color-coded thermal readout in the top-right corner
- **WiFi Configuration**: Configure WiFi and PC IP directly from the touchscreen
- **LibreHardwareMonitor Integration**: Reads all metrics directly via HTTP (`PC_IP:8085/data.json`)
- **PSRAM Support**: Parses large JSON payloads without exhausting DRAM
- **GitHub Actions**: Automated build pipeline with factory.bin generation
