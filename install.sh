#!/bin/bash

# CircuitNotion Arduino Library Installation Script
# This script helps install the CircuitNotion library and its dependencies

echo "=========================================="
echo "CircuitNotion Arduino Library Installer"
echo "=========================================="

# Check if Arduino CLI is installed
if command -v arduino-cli &> /dev/null; then
    echo "✓ Arduino CLI found"
    
    echo "Installing required libraries..."
    
    # Install dependencies
    arduino-cli lib install "ArduinoJson@6.21.3"
    arduino-cli lib install "WebSockets@2.3.7"
    arduino-cli lib install "DHT sensor library@1.4.4"
    
    echo "✓ Dependencies installed"
    echo ""
    echo "To install CircuitNotion library:"
    echo "1. Download the library ZIP from GitHub"
    echo "2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library"
    echo "3. Select the downloaded ZIP file"
    
else
    echo "Arduino CLI not found. Manual installation required:"
    echo ""
    echo "Required Dependencies:"
    echo "- ArduinoJson (6.21.3 or later)"
    echo "- WebSockets (2.3.7 or later)" 
    echo "- DHT sensor library (1.4.4 or later)"
    echo ""
    echo "Installation Steps:"
    echo "1. Open Arduino IDE"
    echo "2. Go to Tools → Manage Libraries"
    echo "3. Search and install each dependency"
    echo "4. Download CircuitNotion library ZIP"
    echo "5. Sketch → Include Library → Add .ZIP Library"
fi

echo ""
echo "=========================================="
echo "Next Steps:"
echo "1. Register at https://circuitnotion.com"
echo "2. Create devices in your dashboard"
echo "3. Add a microcontroller to get API key"
echo "4. Check examples/ folder for getting started"
echo "=========================================="
