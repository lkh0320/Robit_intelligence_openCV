#ifndef cv_hw2_QNODE_HPP_
#define cv_hw2_QNODE_HPP_

#ifndef Q_MOC_RUN
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#endif
#include <QMutex>
#include <QThread>
#include <opencv2/opencv.hpp>

class QNode : public QThread
{
  Q_OBJECT
public:
  QNode();
  ~QNode();
  cv::Mat getImage();

protected:
  void run();

private:
  std::shared_ptr<rclcpp::Node> node;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub;
  cv::Mat image;
  QMutex mutex;

  void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg);

Q_SIGNALS:
  void rosShutDown();
  void imageUpdated();
};

#endif
