#include "../include/cv_hw1/main_window.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <ament_index_cpp/get_package_share_directory.hpp>

static const int VIEW_W = 520;
static const int VIEW_H = 400;

static const int COLOR_PRESET[3][6] = {
  { 170, 10, 100, 255, 80, 255 },
  { 40, 80, 100, 255, 80, 255 },
  { 100, 130, 100, 255, 80, 255 }
};

static const char* COLOR_NAME[3] = { "RED", "GREEN", "BLUE" };

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent), ui(new Ui::MainWindowDesign), color_index_(0), slider_lock_(false)
{
  ui->setupUi(this);

  QIcon icon("://images/icon.png");
  this->setWindowIcon(icon);

  qnode = new QNode();

  QObject::connect(qnode, SIGNAL(rosShutDown()), this, SLOT(close()));

  connect(ui->btnRed, &QPushButton::clicked, this, [this]() { selectColor(0); });
  connect(ui->btnGreen, &QPushButton::clicked, this, [this]() { selectColor(1); });
  connect(ui->btnBlue, &QPushButton::clicked, this, [this]() { selectColor(2); });

  connect(ui->btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
  connect(ui->btnSave, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
  connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::onResetClicked);

  connect(ui->chkGaussian, &QCheckBox::toggled, this, &MainWindow::onSliderChanged);
  connect(ui->sliderKernel, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderHLow, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderHHigh, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderSLow, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderSHigh, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderVLow, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderVHigh, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);

  QString share = QString::fromStdString(ament_index_cpp::get_package_share_directory("cv_hw1"));
  loadImage(share + "/images/balls.png");

  selectColor(0);
}

void MainWindow::selectColor(int index) //선택한 색의 HSV 기본값을 슬라이더에 넣고 다시 이진화
{
  color_index_ = index;

  ui->btnRed->setChecked(index == 0);
  ui->btnGreen->setChecked(index == 1);
  ui->btnBlue->setChecked(index == 2);

  slider_lock_ = true;
  ui->sliderHLow->setValue(COLOR_PRESET[index][0]);
  ui->sliderHHigh->setValue(COLOR_PRESET[index][1]);
  ui->sliderSLow->setValue(COLOR_PRESET[index][2]);
  ui->sliderSHigh->setValue(COLOR_PRESET[index][3]);
  ui->sliderVLow->setValue(COLOR_PRESET[index][4]);
  ui->sliderVHigh->setValue(COLOR_PRESET[index][5]);
  slider_lock_ = false;

  updateSliderLabel();
  updateBinary();
}

void MainWindow::loadImage(const QString& path) //이미지 파일을 읽어서 원본으로 저장
{
  cv::Mat image = cv::imread(path.toStdString());
  if (image.empty())
  {
    ui->labelStatus->setText("이미지를 열지 못했습니다 : " + path);
    return;
  }
  src_ = image;
  updateBinary();
}

cv::Mat MainWindow::makeMask(const cv::Mat& hsv, int h_low, int h_high, int s_low, int s_high, int v_low,
                             int v_high) //HSV 범위로 inRange 하여 바이너리 이미지 생성
{
  cv::Mat mask;
  if (h_low <= h_high)
  {
    cv::inRange(hsv, cv::Scalar(h_low, s_low, v_low), cv::Scalar(h_high, s_high, v_high), mask);
  }
  else
  {
    cv::Mat mask_low;
    cv::Mat mask_high;
    cv::inRange(hsv, cv::Scalar(0, s_low, v_low), cv::Scalar(h_high, s_high, v_high), mask_low);
    cv::inRange(hsv, cv::Scalar(h_low, s_low, v_low), cv::Scalar(179, s_high, v_high), mask_high);
    mask = mask_low | mask_high;
  }
  return mask;
}

void MainWindow::updateBinary() //가우시안 적용 여부에 따라 이진화를 다시 하고 두 화면에 출력
{
  if (src_.empty())
  {
    return;
  }

  int ksize = ui->sliderKernel->value() * 2 + 1;

  if (ui->chkGaussian->isChecked())
  {
    cv::GaussianBlur(src_, view_, cv::Size(ksize, ksize), 0);
    ui->labelOriginalCaption->setText(QString("Original + GaussianBlur %1x%1").arg(ksize));
  }
  else
  {
    view_ = src_.clone();
    ui->labelOriginalCaption->setText("Original");
  }

  cv::Mat hsv;
  cv::cvtColor(view_, hsv, cv::COLOR_BGR2HSV);

  binary_ = makeMask(hsv, ui->sliderHLow->value(), ui->sliderHHigh->value(), ui->sliderSLow->value(),
                     ui->sliderSHigh->value(), ui->sliderVLow->value(), ui->sliderVHigh->value());

  ui->labelBinaryCaption->setText(QString("Binary - %1").arg(COLOR_NAME[color_index_]));

  showImage(ui->labelOriginal, view_);
  showImage(ui->labelBinary, binary_);

  int white = cv::countNonZero(binary_);
  ui->labelStatus->setText(QString("%1x%2  |  선택 색상 %3  |  흰색 픽셀 %4 개  |  가우시안 %5")
                               .arg(src_.cols)
                               .arg(src_.rows)
                               .arg(COLOR_NAME[color_index_])
                               .arg(white)
                               .arg(ui->chkGaussian->isChecked() ? QString("ON (%1x%1)").arg(ksize) : "OFF"));
}

void MainWindow::updateSliderLabel() //슬라이더 옆 숫자 라벨 갱신
{
  ui->labelKernel->setText(QString::number(ui->sliderKernel->value() * 2 + 1));
  ui->labelHLow->setText(QString::number(ui->sliderHLow->value()));
  ui->labelHHigh->setText(QString::number(ui->sliderHHigh->value()));
  ui->labelSLow->setText(QString::number(ui->sliderSLow->value()));
  ui->labelSHigh->setText(QString::number(ui->sliderSHigh->value()));
  ui->labelVLow->setText(QString::number(ui->sliderVLow->value()));
  ui->labelVHigh->setText(QString::number(ui->sliderVHigh->value()));
}

void MainWindow::showImage(QLabel* label, const cv::Mat& image) //cv::Mat 을 QLabel 에 맞춰 출력
{
  if (image.empty())
  {
    label->clear();
    return;
  }

  QImage qimage;
  if (image.channels() == 1)
  {
    qimage = QImage(image.data, image.cols, image.rows, image.step, QImage::Format_Grayscale8);
  }
  else
  {
    qimage = QImage(image.data, image.cols, image.rows, image.step, QImage::Format_BGR888);
  }

  label->setPixmap(QPixmap::fromImage(qimage.copy()).scaled(VIEW_W, VIEW_H, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onOpenClicked() //파일 선택 창을 열어서 다른 이미지 불러오기
{
  QString path = QFileDialog::getOpenFileName(this, "이미지 열기", QString(), "Image (*.png *.jpg *.jpeg *.bmp)");
  if (path.isEmpty())
  {
    return;
  }
  loadImage(path);
}

void MainWindow::onSaveClicked() //현재 바이너리 이미지를 파일로 저장
{
  if (binary_.empty())
  {
    return;
  }

  QString path = QFileDialog::getSaveFileName(this, "바이너리 저장", "binary.png", "Image (*.png)");
  if (path.isEmpty())
  {
    return;
  }

  cv::imwrite(path.toStdString(), binary_);
  ui->labelStatus->setText("저장 완료 : " + path);
}

void MainWindow::onResetClicked() //슬라이더를 현재 색상의 기본값으로 되돌리기
{
  selectColor(color_index_);
}

void MainWindow::onSliderChanged() //슬라이더나 체크박스가 바뀔 때 라벨과 이진화 결과 갱신
{
  if (slider_lock_)
  {
    return;
  }
  updateSliderLabel();
  updateBinary();
}

void MainWindow::closeEvent(QCloseEvent* event) //창을 닫을 때 ROS 종료 후 스레드 정리
{
  if (rclcpp::ok())
  {
    rclcpp::shutdown();
  }
  qnode->wait();
  QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow() //메모리 해제
{
  delete qnode;
  delete ui;
}
