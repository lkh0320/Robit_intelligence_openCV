#ifndef cv_hw1_MAIN_WINDOW_H
#define cv_hw1_MAIN_WINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QString>
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

private:
  Ui::MainWindowDesign* ui;

  cv::Mat src_;
  cv::Mat view_;
  cv::Mat binary_;
  int color_index_;
  bool slider_lock_;

  void selectColor(int index);
  void loadImage(const QString& path);
  void updateBinary();
  void updateSliderLabel();
  void showImage(QLabel* label, const cv::Mat& image);
  cv::Mat makeMask(const cv::Mat& hsv, int h_low, int h_high, int s_low, int s_high, int v_low, int v_high);

  void onOpenClicked();
  void onSaveClicked();
  void onResetClicked();
  void onSliderChanged();

  void closeEvent(QCloseEvent* event);
};

#endif
