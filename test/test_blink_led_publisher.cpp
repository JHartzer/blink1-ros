#include <chrono>
#include <memory>
#include <random>

#include "rclcpp/rclcpp.hpp"
#include "blink1_ros/msg/blink_led.hpp"

class TestBlinkLEDPublisher : public rclcpp::Node
{
public:
  TestBlinkLEDPublisher() : Node("test_blink_led_publisher")
  {
    publisher_ = this->create_publisher<blink1_ros::msg::BlinkLED>("/blink1/set_led", 10);
    
    // Timer running at 1 Hz
    timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&TestBlinkLEDPublisher::timer_callback, this)
    );

    RCLCPP_INFO(this->get_logger(), "Test Blink LED Publisher started. Publishing random colors at 1 Hz...");
  }

private:
  void timer_callback()
  {
    // Random color generator
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

    auto msg = blink1_ros::msg::BlinkLED();
    msg.color.r = distribution(generator);
    msg.color.g = distribution(generator);
    msg.color.b = distribution(generator);
    msg.color.a = 1.0f; // Full brightness
    msg.duration = 0.5; // 0.5 seconds
    msg.device_id = 0;

    RCLCPP_INFO(this->get_logger(), "Publishing color: RGB(%.2f, %.2f, %.2f)", msg.color.r, msg.color.g, msg.color.b);
    publisher_->publish(msg);
  }

  rclcpp::Publisher<blink1_ros::msg::BlinkLED>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TestBlinkLEDPublisher>());
  rclcpp::shutdown();
  return 0;
}
