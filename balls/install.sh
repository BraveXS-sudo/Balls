#!/bin/bash

# ANSI Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}"
echo "  ____    _    _     _     ____  "
echo " | __ )  / \  | |   | |   / ___| "
echo " |  _ \ / _ \ | |   | |   \___ \ "
echo " | |_) / ___ \| |___| |___ ___) |"
echo " |____/_/   \_\_____|_____|____/ "
echo "                                 "
echo -e "${NC}"
echo "Welcome to the BALLS Launcher Installer."
echo "----------------------------------------"

# Check Root
if [[ $EUID -ne 0 ]]; then
   echo -e "${RED}Error: This script must be run as root to install to /usr/local/bin.${NC}" 
   echo "Please run with sudo: sudo ./install.sh"
   exit 1
fi

# Check Deps
echo -e "${BLUE}[*] Checking dependencies...${NC}"
deps=("cmake" "make" "gcc")
for dep in "${deps[@]}"; do
    if ! command -v $dep &> /dev/null; then
        echo -e "${RED}Error: $dep is not installed.${NC}"
        exit 1
    fi
done

# Build
echo -e "${BLUE}[*] Building project...${NC}"
rm -rf build
mkdir -p build
cd build || exit
cmake ..
make
if [ $? -eq 0 ]; then
    echo -e "${GREEN}[+] Build success!${NC}"
else
    echo -e "${RED}[-] Build failed.${NC}"
    exit 1
fi
cd ..

# Cleanup Old Installation
echo -e "${BLUE}[*] Cleaning up old installation...${NC}"
if [ -f "/usr/local/bin/balls" ]; then
    sudo rm -f /usr/local/bin/balls
    echo "  - Removed old binary"
fi
# Remove ALL variants of old icons
sudo rm -f /usr/share/pixmaps/balls-launcher.png
sudo rm -f /usr/share/pixmaps/balls-launcher-v2.png
sudo rm -f /usr/share/icons/hicolor/128x128/apps/balls-launcher.png
sudo rm -f $HOME/.local/share/icons/balls-launcher.png
if [ -f "/usr/share/applications/balls.desktop" ]; then
    sudo rm -f /usr/share/applications/balls.desktop
    echo "  - Removed old desktop entry"
fi

# Install Binary
echo -e "${BLUE}[*] Installing binary to /usr/local/bin/balls...${NC}"
sudo cp build/balls /usr/local/bin/balls
sudo chmod +x /usr/local/bin/balls

# Install Icon (Aggressive Strategy for KDE)
echo -e "${BLUE}[*] Installing icon...${NC}"
if [ -f "icon.png" ]; then
    # 1. /usr/share/pixmaps (Standard)
    sudo cp icon.png /usr/share/pixmaps/balls-launcher-v2.png
    
    # 2. Local Icons (High Priority)
    mkdir -p $HOME/.local/share/icons
    cp icon.png $HOME/.local/share/icons/balls-launcher-v2.png
    
    echo -e "${GREEN}[+] Icon installed (Cache busted).${NC}"
else
    echo -e "${RED}[-] icon.png not found. Skipping icon.${NC}"
fi

# Create Desktop Entry
echo -e "${BLUE}[*] Creating Desktop Entry...${NC}"
sudo cat > /usr/share/applications/balls.desktop << EOL
[Desktop Entry]
Name=Balls Launcher
Comment=The Ultimate Gamery Launcher
Exec=balls
Icon=balls-launcher-v2
Terminal=false
Type=Application
Categories=Game;Utility;
EOL

# Refresh Caches
echo -e "${BLUE}[*] Refreshing System Caches...${NC}"
if command -v update-desktop-database &> /dev/null; then
    sudo update-desktop-database /usr/share/applications
fi
if command -v gtk-update-icon-cache &> /dev/null; then
    # Try to update standard cache, though often restricted
    sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null
fi
# KDE/Generic touch trick
touch $HOME/.local/share/applications


echo -e "${GREEN}[+] Installation Complete!${NC}"
echo "You can now run 'balls' from your terminal or application menu."
echo "Enjoy your BALLS."
