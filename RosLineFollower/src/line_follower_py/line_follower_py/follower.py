import cv2
import numpy as np
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from geometry_msgs.msg import Twist
from cv_bridge import CvBridge

# ── Tuning constants ──────────────────────────────────────────────────────────
KP                 = 0.005   # proportional gain (how hard to steer per pixel of error)
LINEAR_SPEED       = 0.12    # forward speed in m/s
ROI_FRACTION       = 0.40    # use bottom 40 % of image as region of interest
BRIGHTNESS_THRESHOLD = 200   # pixels brighter than this (0–255) = white line
MIN_LINE_PIXELS    = 150     # minimum white pixels to trust the detection


class LineFollowerNode(Node):

    def __init__(self):
        super().__init__('line_follower_node')

        # Subscriber: receive camera images
        self.sub_image = self.create_subscription(
            Image,               # message type
            '/camera/image_raw', # topic name
            self.image_callback, # function to call per message
            10                   # queue size
        )

        # Publishers
        self.pub_cmd   = self.create_publisher(Twist, '/cmd_vel', 10)
        self.pub_debug = self.create_publisher(Image, '/line_image', 10)

        # CvBridge converts ROS Image messages ↔ NumPy arrays
        self.bridge = CvBridge()
        self.get_logger().info('LineFollowerNode started.')

    def image_callback(self, msg: Image):
        # Step 1: ROS Image → OpenCV BGR array
        frame = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        h, w = frame.shape[:2]

        # Step 2: Crop to bottom ROI strip
        roi_y = int(h * (1.0 - ROI_FRACTION))
        roi   = frame[roi_y:, :]

        # Step 3: Grayscale → threshold → white mask
        gray = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
        _, mask = cv2.threshold(gray, BRIGHTNESS_THRESHOLD, 255, cv2.THRESH_BINARY)

        # Step 4: Find centroid of white pixels
        M = cv2.moments(mask)
        if M['m00'] < MIN_LINE_PIXELS:
            self.get_logger().warn('Line lost — stopping.', throttle_duration_sec=1.0)
            self.pub_cmd.publish(Twist())  # zero velocity = stop
            return

        cx = int(M['m10'] / M['m00'])  # centroid X in image pixels

        # Step 5: Error = distance of centroid from image centre
        error = cx - (w // 2)
        # positive error → line is right of centre → turn right (negative angular_z)
        # negative error → line is left of centre  → turn left  (positive angular_z)

        # Step 6: Proportional control
        angular_z = -KP * error
        speed_factor = max(0.4, 1.0 - 0.6 * abs(error) / (w / 2))
        linear_x = LINEAR_SPEED * speed_factor

        # Step 7: Publish velocity command
        cmd = Twist()
        cmd.linear.x  = linear_x
        cmd.angular.z = angular_z
        self.pub_cmd.publish(cmd)


def main(args=None):
    rclpy.init(args=args)
    node = LineFollowerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.pub_cmd.publish(Twist())  # stop robot on exit
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()