# blink1-ros

An unofficial ROS 2 interface for the [blink(1) USB LED](https://blink1.thingm.com/). This package allows you to control one or more connected blink(1) devices using a ROS 2 topic and custom message.

## Description

`blink1_ros` is a ROS 2 package that wraps the low-level `blink1-lib` C library (provided via the [blink1-tool](https://github.com/todbot/blink1-tool) submodule) to interface with **blink(1)** USB LEDs.

### Key Features
* **Topic-Based Interface:** Publish simple ROS 2 messages to instantly control LED colors.
* **Automatic Timers:** Specify a duration in the control message, and the node will automatically turn off the LED after the timer expires using ROS 2 Wall Timers.
* **Brightness Scaling:** Uses the `Alpha` channel of standard `std_msgs/ColorRGBA` to dynamically scale the brightness of the LEDs.
* **Multi-Device Support:** Enumerate and control multiple connected blink(1) units using their integer device indices (`device_id`).

## Build Instructions

### 1. Prerequisites

Make sure you have a working installation of **ROS 2** (e.g., Humble, Iron, or Jazzy) and the necessary development tools installed (including `rosdep`).

### 2. Clone Repository and Submodules

Clone this repository into the `src` directory of your ROS 2 workspace:
```bash
cd /blink1-ws/src
git clone --recursive https://github.com/jacob/blink1-ros.git
```

If you did not clone recursively, navigate to the repository directory and initialize the submodules:
```bash
cd /blink1-ws/src/blink1-ros
git submodule update --init --recursive
```

### 3. Install System Dependencies

Instead of manually installing dependencies, you can automatically resolve and install all system requirements (such as `libudev-dev` and `pkg-config`) using `rosdep` from the root of your ROS 2 workspace:

```bash
cd /blink1-ws
rosdep update
rosdep install --from-paths src --ignore-src -y
```

### 4. Setup USB Permissions (`udev` rules)
By default, standard Linux users do not have permissions to access raw USB HID devices like the `blink(1)` without `sudo`. To enable non-root access:

1. Copy the provided udev rules file:
   ```bash
   sudo cp blink1-tool/51-blink1.rules /etc/udev/rules.d/
   ```
2. Reload and trigger the udev rules:
   ```bash
   sudo udevadm control --reload && sudo udevadm trigger
   ```
3. Re-plug your blink(1) USB device.

### 5. Compiling the Workspace

Navigate to the root of your ROS 2 workspace and build the package using `colcon`:
```bash
cd /blink1-ws
colcon build --symlink-install --packages-select blink1_ros
```

## Usage Instructions

### 1. Run the ROS 2 Node

First, source your workspace overlay:
```bash
source /blink1-ws/install/setup.bash
```

Then start the node:
```bash
ros2 run blink1_ros blink1_node
```

Upon startup, the node will search for connected blink(1) devices and print out a detection log:
```text
[INFO] [blink1_node]: Found 1 blink(1) USB RGB LED device(s) connected
[INFO] [blink1_node]: blink(1) ROS 2 node started. Listening on /blink1/set_led...
```

### 2. Message Interface

The node listens to the `/blink1/set_led` topic using the custom message type `blink1_ros/msg/BlinkLED`.

#### `blink1_ros/msg/BlinkLED` Structure
| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `color` | `std_msgs/ColorRGBA` | None | The color to set the LED to. The fields `r`, `g`, `b` and `a` should be floats in the range `[0.0, 1.0]`. The `a` (alpha) channel scales the brightness. |
| `duration` | `float64` | `0.5` | How long the LED should stay on in seconds. If set to `0.0` or less, the LED stays on indefinitely until a new command is received. |
| `device_id` | `int32` | `0` | The index of the target blink(1) device (useful if multiple devices are plugged in). |

### 3. Example Commands

You can control the LEDs from the command line using `ros2 topic pub`.

#### Turn LED Green for 2 seconds
```bash
ros2 topic pub --once /blink1/set_led blink1_ros/msg/BlinkLED "{color: {r: 0.0, g: 1.0, b: 0.0, a: 1.0}, duration: 2.0, device_id: 0}"
```

#### Flash Red (dimmed to 50% brightness) for 0.5 seconds
```bash
ros2 topic pub --once /blink1/set_led blink1_ros/msg/BlinkLED "{color: {r: 1.0, g: 0.0, b: 0.0, a: 0.5}, duration: 0.5, device_id: 0}"
```

#### Turn LED Blue indefinitely
```bash
ros2 topic pub --once /blink1/set_led blink1_ros/msg/BlinkLED "{color: {r: 0.0, g: 0.0, b: 1.0, a: 1.0}, duration: 0.0, device_id: 0}"
```

#### Turn LED Off manually
```bash
ros2 topic pub --once /blink1/set_led blink1_ros/msg/BlinkLED "{color: {r: 0.0, g: 0.0, b: 0.0, a: 1.0}, duration: 0.0, device_id: 0}"
```

## Running Integration Tests

The package includes an integration test suite powered by `launch_testing` that starts the core `blink1_node` alongside a publisher node (`test_blink_led_publisher`), verifying successful process execution and communication.

To run the integration tests:

```bash
# Navigate to the root of your ROS 2 workspace
cd ~/ros_ws

# Run the tests using colcon
colcon test --packages-select blink1_ros

# View the test results in detail
colcon test-result --all --verbose
```

