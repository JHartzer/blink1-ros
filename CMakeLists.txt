cmake_minimum_required(VERSION 3.8)
project(blink1_ros)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(PkgConfig REQUIRED)

# Find libudev for HIDRAW support
pkg_check_modules(UDEV REQUIRED libudev)

# Generate custom message interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/BlinkLED.msg"
  DEPENDENCIES std_msgs
)

# Declare blink1_node executable
add_executable(blink1_node
  src/blink1_node.cpp
  blink1-tool/blink1-lib.c
  blink1-tool/hidapi/linux/hid.c
)

# Suppress all compiler warnings for third-party vendor C source files
set_source_files_properties(
  blink1-tool/blink1-lib.c
  blink1-tool/hidapi/linux/hid.c
  PROPERTIES COMPILE_FLAGS "-w"
)

# Include paths
target_include_directories(blink1_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${UDEV_INCLUDE_DIRS}
)

# Treat third-party headers as SYSTEM to suppress their warnings in C++ code
target_include_directories(blink1_node SYSTEM PUBLIC
  blink1-tool
  blink1-tool/hidapi/hidapi
)

# Compile definitions for blink1-lib
target_compile_definitions(blink1_node PRIVATE
  USE_HIDAPI=1
)

# Link dependencies
target_link_libraries(blink1_node
  rclcpp::rclcpp
  ${UDEV_LIBRARIES}
  pthread
  rt
  dl
)

# Set dependency on generated message headers (ensure they are generated before compiling the node)
add_dependencies(blink1_node ${PROJECT_NAME}__rosidl_typesupport_cpp)

# Link against the generated typesupport for using custom message in the node
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
target_link_libraries(blink1_node "${cpp_typesupport_target}")

# Declare test_blink_led_publisher executable
add_executable(test_blink_led_publisher test/test_blink_led_publisher.cpp)

# Link dependencies
target_link_libraries(test_blink_led_publisher
  rclcpp::rclcpp
)

# Set dependency on generated message headers (ensure they are generated before compiling the node)
add_dependencies(test_blink_led_publisher ${PROJECT_NAME}__rosidl_typesupport_cpp)

# Link against the generated typesupport for using custom message in the node
target_link_libraries(test_blink_led_publisher "${cpp_typesupport_target}")

# Install targets
install(TARGETS blink1_node test_blink_led_publisher
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(launch_testing_ament_cmake REQUIRED)
  add_launch_test(test/test_blink1_integration.py)
endif()

ament_package()
