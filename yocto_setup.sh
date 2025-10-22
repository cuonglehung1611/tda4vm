#!/bin/bash

# Define the Yocto build directory
YOCTO_BUILD_DIR="/home/$(whoami)/workspace/yocto_build"

# Create the Yocto build directory if it doesn't exist
mkdir -p "$YOCTO_BUILD_DIR"

# Navigate to the Yocto build directory
cd "$YOCTO_BUILD_DIR"

# Clone the Yocto layers repository
git clone https://git.ti.com/git/arago-project/oe-layersetup.git

# Navigate to the cloned repository directory
cd oe-layersetup

# Check the source is exist
if [ -d "$YOCTO_BUILD_DIR/oe-layersetup/build" ]; then
    # Directory exists
    echo "Directory found. Go next"
else
    echo "Directory not found. Start clone source"
	git checkout b17c4b0668d2cc3f068d0cc0293f6760d7c20193
	./oe-layertool-setup.sh -f configs/processor-sdk-analytics/processor-sdk-analytics-11.00.00-config.txt
fi
