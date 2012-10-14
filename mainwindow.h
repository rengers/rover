#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

#include <mediabrowserqlistview.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// Needed for the cascade classifier libraries
#include <opencv2/objdetect/objdetect.hpp>

// opencv C libraries
#include <opencv/cv.h>

#include <vector>

namespace Ui {
class MainWindow;
}

// Modes
const int IMAGE_FILE = 0;
const int VIDEO_FILE = 1;
const int WEBCAM = 2;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public: cv::Mat locatePlate(cv::Mat src);
public:

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();



private slots:
    void on_pauseOrResume_clicked();

    void on_quit_clicked();

    void open();

    void on_sourceSelect_currentIndexChanged(int index);

    void sourceImageChanged(const QModelIndex &index);

    void resetWebcam();

    void on_medianBlurCheckbox_stateChanged(int arg1);

    void on_gaussianBlurCheckbox_stateChanged(int arg1);

    void on_erodeDilateCheckbox_stateChanged(int arg1);

public slots:
    void processFrameAndUpdate();
    cv::Mat drawHist(cv::Mat src, QString s);
    void updateInfo();

private:
    Ui::MainWindow *ui;

    // OpenCV objects
    cv::VideoCapture capWebcam;
    cv::VideoCapture capVideo;
    cv::Mat matOriginal;
    cv::Mat matProcessed;

    // OpenCV values
    int bestThreshold;
    int thresholdValue;

    // Images stored in QImage object
    QImage qimgOriginal;
    QImage qimgProcessed;

    // Holds the filename of the image to be processed
    QString img;

    // Input mode
    int mode;

    // Preprocessor options
    int median_blur_on;
    int gauss_blur_on;
    int erode_dilate_on;

    QFileSystemModel *dirModel;
    QFileSystemModel *fileModel;

    // QT menu related
    QTextEdit *textEdit;

    QAction *openAction;
    QAction *exitAction;

    QMenu *fileMenu;

    // File directory browser
    MediaBrowserQListView* sourceImage;

    // Extension for file types we are interested in
    QStringList image_ext;
    QStringList video_ext;

    // QProcess for tesseract
    QProcess* ocr;

    // Timer to control update
    QTimer* qtimer;

};

#endif // MAINWINDOW_H
