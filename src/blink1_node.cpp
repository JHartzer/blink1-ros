#include <chrono>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/color_rgba.hpp"
#include "blink1_ros/msg/blink_led.hpp"

extern "C" {
#include "blink1-lib.h"
}

class Blink1Node : public rclcpp::Node
{
public:
  Blink1Node() : Node("blink1_node")
  {
    // Initial device scan
    int device_count = blink1_enumerate();
    RCLCPP_INFO(this->get_logger(), "Found %d blink(1) USB RGB LED device(s) connected", device_count);

    // Subscribe to BlinkLED messages
    subscription_ = this->create_subscription<blink1_ros::msg::BlinkLED>(
      "/blink1/set_led",
      10,
      std::bind(&Blink1Node::blink_callback, this, std::placeholders::_1)
    );

    RCLCPP_INFO(this->get_logger(), "blink(1) ROS 2 node started. Listening on /blink1/set_led...");
  }

  ~Blink1Node() override
  {
    RCLCPP_INFO(this->get_logger(), "Shutting down blink(1) node. Turning off all active LEDs...");

    // Cancel all running timers
    for (auto & pair : off_timers_) {
      if (pair.second) {
        pair.second->cancel();
      }
    }
    off_timers_.clear();

    // Reset colors to off and close connections
    for (auto & pair : open_devices_) {
      if (pair.second) {
        blink1_setRGB(pair.second, 0, 0, 0);
        // Using blink1_close_internal directly since blink1_close macro does dev=NULL
        blink1_close_internal(pair.second);
      }
    }
    open_devices_.clear();
  }

private:
  void blink_callback(const blink1_ros::msg::BlinkLED::SharedPtr msg)
  {
    int32_t dev_id = msg->device_id;

    // 1. Get or Open connection to the device
    blink1_device* dev = get_or_open_device(dev_id);
    if (!dev) {
      RCLCPP_ERROR(this->get_logger(), "Cannot perform command: Device ID %d is not available.", dev_id);
      return;
    }

    // 2. Cancel any existing off-timer for this device ID to reset the off time
    auto timer_it = off_timers_.find(dev_id);
    if (timer_it != off_timers_.end()) {
      RCLCPP_DEBUG(this->get_logger(), "Resetting active off-timer for device %d", dev_id);
      timer_it->second->cancel();
      off_timers_.erase(timer_it);
    }

    // 3. Process color with alpha representing brightness (scale RGB by alpha, claming to [0, 255])
    auto scale_and_clamp = [](float channel, float alpha) -> uint8_t {
      float scaled = channel * alpha * 255.0f;
      return static_cast<uint8_t>(std::clamp(scaled, 0.0f, 255.0f));
    };

    uint8_t r = scale_and_clamp(msg->color.r, msg->color.a);
    uint8_t g = scale_and_clamp(msg->color.g, msg->color.a);
    uint8_t b = scale_and_clamp(msg->color.b, msg->color.a);

    RCLCPP_INFO(this->get_logger(), 
      "Device %d: Setting color to RGB(%d, %d, %d) [Alpha: %.2f] for %.2f seconds", 
      dev_id, r, g, b, msg->color.a, msg->duration);

    // 4. Change color immediately
    int rc = blink1_setRGB(dev, r, g, b);
    if (rc == -1) {
      RCLCPP_ERROR(this->get_logger(), "Failed to communicate with blink(1) device %d. Closing handle.", dev_id);
      blink1_close_internal(dev);
      open_devices_.erase(dev_id);
      return;
    }

    // 5. Schedule one-shot timer to turn the LED off after duration
    if (msg->duration > 0.0) {
      auto duration_ms = std::chrono::milliseconds(static_cast<int64_t>(msg->duration * 1000.0));
      
      // Create one-shot timer
      auto timer = this->create_wall_timer(
        duration_ms,
        [this, dev_id]() {
          turn_off_led(dev_id);
        }
      );
      off_timers_[dev_id] = timer;
    }
  }

  blink1_device* get_or_open_device(int32_t dev_id)
  {
    auto it = open_devices_.find(dev_id);
    if (it != open_devices_.end()) {
      return it->second;
    }

    // Device not open yet, enumerate to refresh and attempt to open
    int device_count = blink1_enumerate();
    if (dev_id < 0 || dev_id >= device_count) {
      RCLCPP_ERROR(this->get_logger(), 
        "Requested device index %d is out of bounds. Available connected devices: %d", 
        dev_id, device_count);
      return nullptr;
    }

    blink1_device* dev = blink1_openById(static_cast<uint32_t>(dev_id));
    if (!dev) {
      RCLCPP_ERROR(this->get_logger(), "Could not open blink(1) device ID %d", dev_id);
      return nullptr;
    }

    RCLCPP_INFO(this->get_logger(), "Successfully opened connection to blink(1) device ID %d", dev_id);
    open_devices_[dev_id] = dev;
    return dev;
  }

  void turn_off_led(int32_t dev_id)
  {
    // Find device and turn it off
    auto dev_it = open_devices_.find(dev_id);
    if (dev_it != open_devices_.end() && dev_it->second) {
      RCLCPP_INFO(this->get_logger(), "Duration elapsed. Turning off device %d", dev_id);
      blink1_setRGB(dev_it->second, 0, 0, 0);
    }

    // Clean up and cancel the timer that just completed
    auto timer_it = off_timers_.find(dev_id);
    if (timer_it != off_timers_.end()) {
      timer_it->second->cancel();
      off_timers_.erase(timer_it);
    }
  }

  rclcpp::Subscription<blink1_ros::msg::BlinkLED>::SharedPtr subscription_;
  std::unordered_map<int32_t, blink1_device*> open_devices_;
  std::unordered_map<int32_t, rclcpp::TimerBase::SharedPtr> off_timers_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Blink1Node>());
  rclcpp::shutdown();
  return 0;
}
