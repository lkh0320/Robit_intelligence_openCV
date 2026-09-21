#include "../include/cv_hw2/qnode.hpp"

QNode::QNode() //ROS 초기화, /image_raw 구독 생성, 스레드 시작
{
  int argc = 0;
  char** argv = NULL;
  rclcpp::init(argc, argv);
  node = rclcpp::Node::make_shared("cv_hw2");

  image_sub = node->create_subscription<sensor_msgs::msg::Image>(
      "/image_raw", 10, std::bind(&QNode::imageCallback, this, std::placeholders::_1));

  this->start();
}

QNode::~QNode() //ROS 종료
{
  if (rclcpp::ok())
  {
    rclcpp::shutdown();
  }
}

void QNode::imageCallback(const sensor_msgs::msg::Image::SharedPtr msg) //받은 Image 메시지를 cv::Mat 으로 변환해서 저장
{
  if (msg->encoding != "bgr8" && msg->encoding != "rgb8")
  {
    return;
  }

  cv::Mat received(msg->height, msg->width, CV_8UC3, msg->data.data(), msg->step);
  cv::Mat converted;
  if (msg->encoding == "rgb8")
  {
    cv::cvtColor(received, converted, cv::COLOR_RGB2BGR);
  }
  else
  {
    converted = received.clone();
  }

  mutex.lock();
  image = converted;
  mutex.unlock();

  Q_EMIT imageUpdated();
}

cv::Mat QNode::getImage() //저장된 최신 프레임 복사해서 반환
{
  mutex.lock();
  cv::Mat copy = image.clone();
  mutex.unlock();
  return copy;
}

void QNode::run() //30Hz로 spin 하다가 종료되면 시그널 발생
{
  rclcpp::WallRate loop_rate(30);
  while (rclcpp::ok())
  {
    rclcpp::spin_some(node);
    loop_rate.sleep();
  }
  rclcpp::shutdown();
  Q_EMIT rosShutDown();
}
