#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace Ui {
class MainWindow;
}

// Modes
const int WEBCAM = 0;
const int IMAGE_FILE = 1;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();



private slots:
    void on_pauseOrResume_clicked();

    void on_quit_clicked();

    void open();

    void on_sourceDir_clicked(const QModelIndex &index);

    void on_sourceSelect_currentIndexChanged(int index);

public slots:
    void processFrameAndUpdate();

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

    int mode;

    QFileSystemModel *dirModel;
    QFileSystemModel *fileModel;

    // QT menu related
    QTextEdit *textEdit;

    QAction *openAction;
    QAction *exitAction;

    QMenu *fileMenu;

    // Timer to control update
    QTimer* qtimer;

};

#endif // MAINWINDOW_H
