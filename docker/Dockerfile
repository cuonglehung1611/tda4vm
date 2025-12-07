# Use the official Ubuntu 22.04 as a base image
FROM ubuntu:22.04

# Preconfigure the system to use bash as the default shell
SHELL ["/bin/bash", "-c"]

# Install basic system packages
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    bash sudo locales curl debconf-utils debianutils aptitude

# Configure locale
RUN locale-gen en_US en_US.UTF-8 \
    && update-locale LANG=en_US.UTF-8

# It is recommended that USER_NAME, UID, and GID match the host user to avoid permission problems
# while using the container. Default values are set in case build command arguments are missing
ARG UID=1000
ARG GID=1000
ARG USER_NAME=cuong

RUN groupadd --gid $GID $USER_NAME \
    && useradd --uid $UID --gid $GID --create-home --shell /bin/bash $USER_NAME \
    && echo "$USER_NAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers \
    && touch /home/$USER_NAME/.bashrc \
    && echo "export LANG=en_US.UTF-8" >> /home/$USER_NAME/.bashrc

# Installing base software development packages
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    autoconf automake build-essential make cmake g++ gcc git git-lfs python3 python3-pip \
    python3-dev python3-numpy python3-venv

# Installing packages needed for Yocto & SDK Makefile builds
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    file iproute2 usbutils dialog dfu-util libssl-dev bc u-boot-tools swig wget \
    device-tree-compiler dosfstools fdisk gawk diffstat unzip texinfo chrpath socat cpio \
    python3-pexpect xz-utils python3-git python3-jinja2 libegl1-mesa libsdl1.2-dev xterm pylint \
    python3-subunit mesa-common-dev zstd liblz4-tool doxygen dos2unix bison flex mono-devel \
    mono-complete python3-distutils repo pseudo python3-sphinx g++-multilib libc6-dev-i386 \
    jq pigz liblz4-tool lz4 net-tools zip corkscrew rsync iputils-ping libacl1 apt-utils nano \
    tmux xfce4-terminal openssh-server vim \
    && pip3 install jsonschema pyelftools PyYAML Mako yamllint

# Install packages necessary for running C7000 Host Emulation
# g++ > 9.0.0 should work. Ubuntu 22.04 has g++ > 11.4.0
RUN DEBIAN_FRONTEND=noninteractive wget -P \
 /opt/ti_cgt_c71x https://dr-download.ti.com/software-development/ide-configuration-compiler-or-debugger/MD-707zYe3Rik/4.1.1.LTS/ti_cgt_c7000_4.1.1.LTS_linux-x64_installer.bin \
    && cd /opt/ti_cgt_c71x \
    && chmod +x ti_cgt_c7000_4.1.1.LTS_linux-x64_installer.bin \
    && ./ti_cgt_c7000_4.1.1.LTS_linux-x64_installer.bin  --mode unattended --installdir . \
    && rm ti_cgt_c7000_4.1.1.LTS_linux-x64_installer.bin

# Install GUI dependencies BEFORE building OpenCV
RUN apt-get update && apt-get install -y \
    libgtk-3-dev \
    pkg-config \
    libavcodec-dev libavformat-dev libswscale-dev \
    libv4l-dev \
    libxvidcore-dev libx264-dev \
    libjpeg-dev libpng-dev libtiff-dev \
    gfortran \
    libtbb-dev \
    libatlas-base-dev \
    && rm -rf /var/lib/apt/lists/*

# Install opencv v4.5.5
RUN cd /tmp && mkdir ~/opencv_build && cd ~/opencv_build \
    && git clone https://github.com/opencv/opencv.git \
    && git clone https://github.com/opencv/opencv_contrib.git \
    && cd ~/opencv_build/opencv/ \
    && git checkout tags/4.5.5 -b v4.5.5-branch \
    && cd ~/opencv_build/opencv_contrib/ \
    && git checkout tags/4.5.5 -b v4.5.5-branch \
    && mkdir -p ~/opencv_build/opencv/build && cd ~/opencv_build/opencv/build \
	&& cmake -D CMAKE_BUILD_TYPE=Release \
		-D CMAKE_INSTALL_PREFIX=/usr/local \
		-D INSTALL_C_EXAMPLES=ON \
		-D INSTALL_PYTHON_EXAMPLES=ON \
		-D OPENCV_GENERATE_PKGCONFIG=ON \
		-D BUILD_EXAMPLES=ON \
		-D WITH_TBB=ON \
		-D WITH_V4L=ON \
		-D WITH_OPENGL=ON \
		-D WITH_GSTREAMER=ON \
		-D WITH_ONNX=ON \
		-D WITH_GTK=ON \
        -D BUILD_opencv_python2=OFF \
        -D BUILD_opencv_python3=OFF \
		-D OPENCV_ENABLE_NONFREE=ON \
		-D OPENCV_EXTRA_MODULES_PATH=~/opencv_build/opencv_contrib/modules .. \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && pkg-config --modversion opencv4

# Set up Python dependencies for the algorithm build system (check adasvf/python-setup/default)
COPY pip_package /tmp/pip_package
RUN DEBIAN_FRONTEND=noninteractive cd /tmp/pip_package \
    && pip3 install -r requirements.txt

# Clean installation
RUN DEBIAN_FRONTEND=noninteractive aptitude clean && rm -rf /tmp/*

WORKDIR /home/$USER_NAME



