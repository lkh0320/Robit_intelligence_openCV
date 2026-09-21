#ifndef cv_hw2_MAIN_WINDOW_H
#define cv_hw2_MAIN_WINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <QVector>
#include <opencv2/opencv.hpp>

#include "QIcon"
#include "qnode.hpp"
#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();
  QNode* qnode;

  enum Target
  {
    WHITE_LINE = 0,
    BLUE_LINE = 1,
    NEON_CONE = 2,
    ORANGE_CONE = 3,
    TARGET_COUNT = 4
  };

private:
  Ui::MainWindowDesign* ui;

  int hsv_value_[TARGET_COUNT][6];
  int target_;
  bool slider_lock_;
  bool combo_lock_;

  cv::Mat frame_;
  cv::Mat result_;
  cv::Mat mask_[TARGET_COUNT];
  cv::Mat crop_neon_;
  cv::Mat crop_orange_;

  cv::VideoCapture cap_;
  QTimer* timer_;
  QVector<int> camera_list_;

  void scanCamera();
  QString cameraName(int index);
  void openCamera(int combo_index);

  void selectTarget(int target);
  void loadSliderValue();
  void saveSliderValue();
  void updateSliderLabel();
  void setFrame(const cv::Mat& frame);
  void process();
  cv::Mat makeMask(const cv::Mat& hsv, int target);
  cv::Rect findObject(const cv::Mat& mask);
  QString judgePosition(const cv::Rect& object, const cv::Rect& white, const cv::Rect& blue);
  void showImage(QLabel* label, const cv::Mat& image, int width, int height);

  void onSliderChanged();
  void onCameraChanged(int index);
  void onRefreshClicked();
  void onResetClicked();
  void onTimer();
  void onImageUpdated();

  void closeEvent(QCloseEvent* event);
};

#endif
