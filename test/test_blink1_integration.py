import os
import time
import unittest
import launch
import launch_testing
import launch_testing.actions
import launch_ros.actions

import launch_testing.asserts

def generate_test_description():
    # Node to control the blink(1) USB RGB LED device
    blink1_node = launch_ros.actions.Node(
        package='blink1_ros',
        executable='blink1_node',
        output='screen'
    )

    # Test publisher node that blinks a different color at 1 Hz
    test_blink_led_publisher = launch_ros.actions.Node(
        package='blink1_ros',
        executable='test_blink_led_publisher',
        output='screen'
    )

    return launch.LaunchDescription([
        blink1_node,
        test_blink_led_publisher,
        launch_testing.actions.ReadyToTest(),
    ]), {
        'blink1_node': blink1_node,
        'test_blink_led_publisher': test_blink_led_publisher
    }

class TestBlink1Integration(unittest.TestCase):

    def test_nodes_run(self):
        # Let the integration run for 5 seconds to allow message exchange
        time.sleep(5.0)

@launch_testing.post_shutdown_test()
class TestBlink1Shutdown(unittest.TestCase):

    def test_exit_codes(self, proc_info):
        # Check that all processes exited with code 0 (clean shutdown)
        launch_testing.asserts.assertExitCodes(proc_info)

