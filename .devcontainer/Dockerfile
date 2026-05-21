FROM ros:jazzy-ros-base

# Install additional system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libusb-1.0-0-dev \
    libudev-dev \
    pkg-config \
    udev \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Create the workspace directory structure
RUN mkdir -p /blink1-ws/src

# Source the ROS 2 setup in bashrc for convenience
RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc \
    && echo "if [ -f /blink1-ws/install/setup.bash ]; then source /blink1-ws/install/setup.bash; fi" >> /root/.bashrc
