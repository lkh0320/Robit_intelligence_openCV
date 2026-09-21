#include "../include/cv_hw1/qnode.hpp"

QNode::QNode() //ROS 초기화 후 노드 생성, 스레드 시작
{
  int argc = 0;
  char** argv = NULL;
  rclcpp::init(argc, argv);
  node = rclcpp::Node::make_shared("cv_hw1");
  this->start();
}

QNode::~QNode() //ROS 종료
{
  if (rclcpp::ok())
  {
    rclcpp::shutdown();
  }
}

void QNode::run() //20Hz로 spin 하다가 종료되면 시그널 발생
{
  rclcpp::WallRate loop_rate(20);
  while (rclcpp::ok())
  {
    rclcpp::spin_some(node);
    loop_rate.sleep();
  }
  rclcpp::shutdown();
  Q_EMIT rosShutDown();
}

