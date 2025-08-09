# Changelog

All notable changes to the CircuitNotion Arduino Library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-08-09

### Added

- Initial release of CircuitNotion Arduino Library
- WebSocket-based real-time communication with CircuitNotion platform
- Support for ESP8266 and ESP32 microcontrollers
- Device mapping for local pin control
- Sensor management for temperature, humidity, light, motion, and custom sensors
- Automatic reconnection handling
- Change detection for optimized sensor readings
- Comprehensive error handling and diagnostics
- Production-ready examples (Basic, Realistic, Advanced)
- Complete API documentation
- Arduino Library Manager compatibility

### Architecture

- Implemented dashboard-first device management approach
- Microcontroller authentication via API keys
- Proper separation between device control and sensor data collection
- Memory-efficient implementation for resource-constrained devices

### Examples

- **BasicExample**: Simple getting started example
- **RealisticExample**: Complete real-world IoT setup with DHT22 sensor
- **AdvancedExample**: Comprehensive feature demonstration

### Documentation

- Complete README with API reference
- Architecture correction documentation
- Troubleshooting guide
- Hardware requirements and setup instructions

## [Unreleased]

### Planned

- ESP32-CAM support for image sensors
- LoRaWAN integration for long-range communication
- Edge AI integration for local processing
- Over-the-air (OTA) update support
- Advanced security features
