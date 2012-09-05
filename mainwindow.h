#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

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
const int WEBCAM = 0;
const int IMAGE_FILE = 1;


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

    void on_sourceImage_clicked(const QModelIndex &index);

    void on_sourceSelect_currentIndexChanged(int index);

    void on_checkBox_stateChanged(int arg1);

    void on_sourceImage_activated(const QModelIndex &index);

public slots:
    void processFrameAndUpdate();
    void updateInfo();

private:
    Ui::MainWindow *ui;

    // OpenCV objects
    cv::VideoCapture capWebcam;
    cv::Mat matOriginal;
    cv::Mat matProcessed;

    // Images stored in QImage object
    QImage qimgOriginal;
    QImage qimgProcessed;

    // Holds the filename of the image to be processed
    QString img;

    // Input mode
    int mode;

    // Median filter blur option
    int blur_on;

    QFileSystemModel *dirModel;
    QFileSystemModel *fileModel;

    // QT menu related
    QTextEdit *textEdit;

    QAction *openAction;
    QAction *exitAction;

    QMenu *fileMenu;

    // QProcess for tesseract
    QProcess* ocr;

    // Timer to control update
    QTimer* qtimer;

};

#endif // MAINWINDOW_H
