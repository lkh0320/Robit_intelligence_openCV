#include "../include/cv_hw2/main_window.hpp"

#include <QDir>
#include <QFile>
#include <algorithm>

static const int VIEW_W = 300;
static const int VIEW_H = 225;
static const int CROP_W = 150;
static const int CROP_H = 115;

static const int MIN_AREA = 300;
static const int TIMER_MS = 33;

static const int HSV_PRESET[4][6] = {
  { 0, 179, 0, 0, 250, 255 },
  { 90, 115, 90, 255, 190, 255 },
  { 28, 55, 20, 255, 150, 255 },
  { 0, 30, 70, 255, 225, 255 }
};

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent), ui(new Ui::MainWindowDesign), target_(0), slider_lock_(false), combo_lock_(false)
{
  ui->setupUi(this);

  QIcon icon("://images/icon.png");
  this->setWindowIcon(icon);

  for (int i = 0; i < TARGET_COUNT; i++)
  {
    for (int j = 0; j < 6; j++)
    {
      hsv_value_[i][j] = HSV_PRESET[i][j];
    }
  }

  qnode = new QNode();

  QObject::connect(qnode, SIGNAL(rosShutDown()), this, SLOT(close()));
  connect(qnode, &QNode::imageUpdated, this, &MainWindow::onImageUpdated);

  connect(ui->radioWhite, &QRadioButton::clicked, this, [this]() { selectTarget(WHITE_LINE); });
  connect(ui->radioBlue, &QRadioButton::clicked, this, [this]() { selectTarget(BLUE_LINE); });
  connect(ui->radioNeon, &QRadioButton::clicked, this, [this]() { selectTarget(NEON_CONE); });
  connect(ui->radioOrange, &QRadioButton::clicked, this, [this]() { selectTarget(ORANGE_CONE); });

  connect(ui->sliderHLow, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderHHigh, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderSLow, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderSHigh, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderVLow, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);
  connect(ui->sliderVHigh, &QSlider::valueChanged, this, &MainWindow::onSliderChanged);

  connect(ui->comboCamera, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onCameraChanged);
  connect(ui->btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
  connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::onResetClicked);

  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &MainWindow::onTimer);

  selectTarget(WHITE_LINE);

  scanCamera();
  openCamera(ui->comboCamera->currentIndex());
}

void MainWindow::scanCamera() //dev 폴더에서 video 장치를 찾아 실제로 열리는 것만 목록에 넣기
{
  combo_lock_ = true;
  ui->comboCamera->clear();
  camera_list_.clear();

  QVector<int> found;
  QDir dir("/dev");
  QStringList entry = dir.entryList(QStringList() << "video*", QDir::AllEntries | QDir::System);
  for (int i = 0; i < entry.size(); i++)
  {
    int index = entry[i].mid(5).toInt();
    if (!found.contains(index))
    {
      found.append(index);
    }
  }
  std::sort(found.begin(), found.end());

  for (int i = 0; i < found.size(); i++)
  {
    cv::VideoCapture test;
    if (!test.open(found[i], cv::CAP_V4L2))
    {
      continue;
    }
    test.release();
    camera_list_.append(found[i]);
    ui->comboCamera->addItem(QString("Camera %1 - %2").arg(found[i]).arg(cameraName(found[i])));
  }

  camera_list_.append(-1);
  ui->comboCamera->addItem("ROS /image_raw");

  combo_lock_ = false;

  if (camera_list_.size() == 1)
  {
    ui->labelStatus->setText("연결된 카메라 없음");
  }
}

QString MainWindow::cameraName(int index) //sys 폴더에서 카메라 이름 읽기
{
  QFile file(QString("/sys/class/video4linux/video%1/name").arg(index));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    return QString("video%1").arg(index);
  }
  QString name = QString::fromUtf8(file.readLine()).trimmed();
  file.close();
  return name.isEmpty() ? QString("video%1").arg(index) : name;
}

void MainWindow::openCamera(int combo_index) //선택한 장치를 열고 타이머 시작
{
  timer_->stop();
  if (cap_.isOpened())
  {
    cap_.release();
  }

  if (combo_index < 0 || combo_index >= camera_list_.size())
  {
    return;
  }

  int device = camera_list_[combo_index];
  if (device < 0)
  {
    ui->labelStatus->setText("ROS /image_raw 수신 대기 중");
    return;
  }

  if (!cap_.open(device, cv::CAP_V4L2))
  {
    ui->labelStatus->setText(QString("/dev/video%1 열기 실패").arg(device));
    return;
  }

  cap_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
  cap_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

  timer_->start(TIMER_MS);
  ui->labelStatus->setText(QString("/dev/video%1 연결됨").arg(device));
}

void MainWindow::selectTarget(int target) //슬라이더가 조절할 대상을 바꾸고 저장된 값을 불러오기
{
  target_ = target;
  loadSliderValue();
  updateSliderLabel();
  process();
}

void MainWindow::loadSliderValue() //현재 대상의 HSV 값을 슬라이더에 넣기
{
  slider_lock_ = true;
  ui->sliderHLow->setValue(hsv_value_[target_][0]);
  ui->sliderHHigh->setValue(hsv_value_[target_][1]);
  ui->sliderSLow->setValue(hsv_value_[target_][2]);
  ui->sliderSHigh->setValue(hsv_value_[target_][3]);
  ui->sliderVLow->setValue(hsv_value_[target_][4]);
  ui->sliderVHigh->setValue(hsv_value_[target_][5]);
  slider_lock_ = false;
}

void MainWindow::saveSliderValue() //슬라이더 값을 현재 대상에 저장
{
  hsv_value_[target_][0] = ui->sliderHLow->value();
  hsv_value_[target_][1] = ui->sliderHHigh->value();
  hsv_value_[target_][2] = ui->sliderSLow->value();
  hsv_value_[target_][3] = ui->sliderSHigh->value();
  hsv_value_[target_][4] = ui->sliderVLow->value();
  hsv_value_[target_][5] = ui->sliderVHigh->value();
}

void MainWindow::updateSliderLabel() //슬라이더 옆 숫자 라벨 갱신
{
  ui->labelHLow->setText(QString::number(ui->sliderHLow->value()));
  ui->labelHHigh->setText(QString::number(ui->sliderHHigh->value()));
  ui->labelSLow->setText(QString::number(ui->sliderSLow->value()));
  ui->labelSHigh->setText(QString::number(ui->sliderSHigh->value()));
  ui->labelVLow->setText(QString::number(ui->sliderVLow->value()));
  ui->labelVHigh->setText(QString::number(ui->sliderVHigh->value()));
}

void MainWindow::setFrame(const cv::Mat& frame) //새 프레임을 받아서 처리
{
  if (frame.empty())
  {
    return;
  }
  frame_ = frame.clone();
  process();
}

cv::Mat MainWindow::makeMask(const cv::Mat& hsv, int target) //HSV 범위로 inRange 후 모폴로지로 노이즈 제거
{
  int h_low = hsv_value_[target][0];
  int h_high = hsv_value_[target][1];
  int s_low = hsv_value_[target][2];
  int s_high = hsv_value_[target][3];
  int v_low = hsv_value_[target][4];
  int v_high = hsv_value_[target][5];

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

  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
  cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
  cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
  return mask;
}

cv::Rect MainWindow::findObject(const cv::Mat& mask) //흰색 픽셀이 충분하면 그 영역의 바운딩박스 반환
{
  if (cv::countNonZero(mask) < MIN_AREA)
  {
    return cv::Rect();
  }
  return cv::boundingRect(mask);
}

QString MainWindow::judgePosition(const cv::Rect& object, const cv::Rect& white,
                                  const cv::Rect& blue) //흰선과 파란선을 기준으로 콘의 위치를 10가지 중 하나로 판별
{
  if (object.area() == 0)
  {
    return "존재하지 않음";
  }
  if (white.area() == 0 || blue.area() == 0)
  {
    return "기준선 없음";
  }

  int cx = object.x + object.width / 2;
  int cy = object.y + object.height / 2;
  int white_cx = white.x + white.width / 2;
  int blue_cy = blue.y + blue.height / 2;

  bool on_white = (cx >= white.x && cx <= white.x + white.width);
  bool on_blue = (cy >= blue.y && cy <= blue.y + blue.height);

  if (on_white && on_blue)
  {
    return "중앙";
  }
  if (on_white)
  {
    return (cy < blue_cy) ? "위쪽 하얀선 위" : "아래쪽 하얀선 위";
  }
  if (on_blue)
  {
    return (cx < white_cx) ? "왼쪽 파란선 위" : "오른쪽 파란선 위";
  }
  if (cx > white_cx && cy < blue_cy)
  {
    return "1사분면";
  }
  if (cx < white_cx && cy < blue_cy)
  {
    return "2사분면";
  }
  if (cx < white_cx && cy > blue_cy)
  {
    return "3사분면";
  }
  return "4사분면";
}

void MainWindow::process() //한 프레임을 4가지 색으로 이진화하고 콘 위치 판별 후 화면에 출력
{
  if (frame_.empty())
  {
    return;
  }

  cv::Mat blurred;
  cv::Mat hsv;
  cv::GaussianBlur(frame_, blurred, cv::Size(5, 5), 0);
  cv::cvtColor(blurred, hsv, cv::COLOR_BGR2HSV);

  for (int i = 0; i < TARGET_COUNT; i++)
  {
    mask_[i] = makeMask(hsv, i);
  }

  cv::Rect white_rect = findObject(mask_[WHITE_LINE]);
  cv::Rect blue_rect = findObject(mask_[BLUE_LINE]);
  cv::Rect neon_rect = findObject(mask_[NEON_CONE]);
  cv::Rect orange_rect = findObject(mask_[ORANGE_CONE]);

  result_ = frame_.clone();

  if (white_rect.area() > 0)
  {
    int cx = white_rect.x + white_rect.width / 2;
    cv::line(result_, cv::Point(cx, 0), cv::Point(cx, result_.rows), cv::Scalar(0, 255, 255), 2);
  }
  if (blue_rect.area() > 0)
  {
    int cy = blue_rect.y + blue_rect.height / 2;
    cv::line(result_, cv::Point(0, cy), cv::Point(result_.cols, cy), cv::Scalar(255, 0, 255), 2);
  }

  if (neon_rect.area() > 0)
  {
    cv::rectangle(result_, neon_rect, cv::Scalar(0, 255, 0), 2);
    crop_neon_ = frame_(neon_rect).clone();
  }
  else
  {
    crop_neon_.release();
  }

  if (orange_rect.area() > 0)
  {
    cv::rectangle(result_, orange_rect, cv::Scalar(0, 128, 255), 2);
    crop_orange_ = frame_(orange_rect).clone();
  }
  else
  {
    crop_orange_.release();
  }

  ui->labelNeonPos->setText(QString("Neon_cone : ") + judgePosition(neon_rect, white_rect, blue_rect));
  ui->labelOrangePos->setText(QString("Orange_cone : ") + judgePosition(orange_rect, white_rect, blue_rect));

  showImage(ui->labelSource, frame_, VIEW_W, VIEW_H);
  showImage(ui->labelResult, result_, VIEW_W, VIEW_H);
  showImage(ui->labelNeon, mask_[NEON_CONE], VIEW_W, VIEW_H);
  showImage(ui->labelWhite, mask_[WHITE_LINE], VIEW_W, VIEW_H);
  showImage(ui->labelBlue, mask_[BLUE_LINE], VIEW_W, VIEW_H);
  showImage(ui->labelOrange, mask_[ORANGE_CONE], VIEW_W, VIEW_H);
  showImage(ui->labelNeonCrop, crop_neon_, CROP_W, CROP_H);
  showImage(ui->labelOrangeCrop, crop_orange_, CROP_W, CROP_H);
}

void MainWindow::showImage(QLabel* label, const cv::Mat& image, int width, int height) //cv::Mat 을 QLabel 에 맞춰 출력
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

  label->setPixmap(QPixmap::fromImage(qimage.copy()).scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onSliderChanged() //슬라이더가 바뀌면 값을 저장하고 다시 처리
{
  if (slider_lock_)
  {
    return;
  }
  saveSliderValue();
  updateSliderLabel();
  process();
}

void MainWindow::onCameraChanged(int index) //콤보박스에서 다른 장치를 고르면 그 장치로 교체
{
  if (combo_lock_)
  {
    return;
  }
  openCamera(index);
}

void MainWindow::onRefreshClicked() //카메라 목록 다시 검색
{
  timer_->stop();
  if (cap_.isOpened())
  {
    cap_.release();
  }
  scanCamera();
  openCamera(ui->comboCamera->currentIndex());
}

void MainWindow::onResetClicked() //현재 대상의 슬라이더를 기본값으로 되돌리기
{
  for (int j = 0; j < 6; j++)
  {
    hsv_value_[target_][j] = HSV_PRESET[target_][j];
  }
  selectTarget(target_);
}

void MainWindow::onTimer() //카메라에서 프레임 읽기
{
  cv::Mat frame;
  cap_ >> frame;
  setFrame(frame);
}

void MainWindow::onImageUpdated() //ROS 토픽으로 받은 프레임 처리
{
  int index = ui->comboCamera->currentIndex();
  if (index < 0 || index >= camera_list_.size() || camera_list_[index] >= 0)
  {
    return;
  }
  setFrame(qnode->getImage());
}

void MainWindow::closeEvent(QCloseEvent* event) //창을 닫을 때 카메라와 ROS 정리
{
  timer_->stop();
  if (cap_.isOpened())
  {
    cap_.release();
  }
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
