#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon("web.png"));

    // Set up webcam
    capWebcam.open(0);
    capWebcam.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    capWebcam.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

    // Connect to webcam
    if(capWebcam.isOpened() == false) {
        ui->info->appendPlainText("Error: cannot connect to webcam");
        return;
    }

    // Set up QT menu and UI
    openAction = new QAction(tr("&Open"), this);
    exitAction = new QAction(tr("E&xit"), this);
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    textEdit = new QTextEdit;
    //setCentralWidget(textEdit);

    // Create the input file chooser
    dirModel = new QFileSystemModel(this);
    fileModel = new QFileSystemModel(this);
    fileModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    fileModel->setNameFilters(QStringList("*.jpg"));
    fileModel->setNameFilterDisables(false);

    QString sPath = QDir::currentPath();
    QModelIndex idx = fileModel->setRootPath(sPath.append("/img"));

    ui->sourceDir->setModel(fileModel);
    ui->sourceDir->setRootIndex(idx);
    ui->sourceDir->hide();

   // ui->sourceDir->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);

    setWindowTitle(tr("Rover"));

    // Set up timer to control update
    qtimer = new QTimer(this);
    connect(qtimer, SIGNAL(timeout()), this, SLOT(processFrameAndUpdate()));
    qtimer->start(50);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                                    tr("Text Files (*.txt);;C++ Files (*.cpp *.h)"));

    if (fileName != "") {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Could not open file"));
            return;
        }
        QTextStream in(&file);
        textEdit->setText(in.readAll());
        file.close();
    }
}


void MainWindow::processFrameAndUpdate() {

    // Check if we are geting input from webcam or file
    if( ui->sourceSelect->currentIndex() == 0)
        capWebcam.read(matOriginal);

    else
       matOriginal = cv::imread(img.toStdString());

    if(matOriginal.empty() == true)
        return;

    cv::resize(matOriginal, matOriginal, cv::Size(640, 480));

    //cv::GaussianBlur(matOriginal, matProcessed, cv::Size(9,9), 1.5);

    // Make a copy of the matrix to process
    matOriginal.copyTo(matProcessed);

    if(blur_on)
        cv::medianBlur(matOriginal, matProcessed, 3);

    matProcessed = locatePlate(matProcessed);
    //cv::cvtColor(matProcessed, matProcessed, CV_GRAY2RGB);

    cv::cvtColor(matOriginal, matOriginal, CV_BGR2RGB);
    cv::cvtColor(matProcessed, matProcessed, CV_BGR2RGB);

    QImage qimgOriginal((uchar*)matOriginal.data, matOriginal.cols, matOriginal.rows, matOriginal.step, QImage::Format_RGB888);
    QImage qimgProcessed((uchar*)matProcessed.data, matProcessed.cols, matProcessed.rows, matProcessed.step, QImage::Format_RGB888);

    qimgOriginal = qimgOriginal.scaledToWidth(320);
    qimgProcessed = qimgProcessed.scaledToWidth(320);

    ui->original->setPixmap(QPixmap::fromImage(qimgOriginal).scaledToWidth(320));
    ui->processed->setPixmap(QPixmap::fromImage(qimgProcessed));

    if(mode == IMAGE_FILE)
        qtimer->stop();

}

// This method will locate a possible plate in a image
//
// Input - cv::Mat image to be processed
// Output - cv::Mat image of the ROI
cv::Mat MainWindow::locatePlate(cv::Mat src) {

    cv::Mat dest;
    cv::cvtColor(src, src, CV_RGB2BGR);
    cv::cvtColor(src, dest, CV_BGR2GRAY);

    cv::CascadeClassifier plateLocator;
    plateLocator.load("Numberplate.xml");

    double scale = 1.1;

    std::vector<cv::Rect> plates;
    plateLocator.detectMultiScale(dest, plates, scale, 3, cv::CASCADE_DO_CANNY_PRUNING, cv::Size(90, 20));

    cv::Rect *r;
    for(int i = 0; i < (int)plates.size(); i++) {
        r = &(plates[i]);
        cv::rectangle(src, *r, cv::Scalar(255, 50, 50),3);
    }

    cv::cvtColor(src, src, CV_BGR2RGB);
    return src;
}

void MainWindow::on_pauseOrResume_clicked()
{
    if(qtimer->isActive() == true ) {
     qtimer->stop();
     ui->pauseOrResume->setText("Resume");
    }
    else {
        qtimer->start();
        ui->pauseOrResume->setText("Pause");
    }

}

void MainWindow::on_quit_clicked()
{
    exit(0);
}

void MainWindow::on_sourceDir_clicked(const QModelIndex &index)
{
    // Update image
    img = fileModel->fileInfo(index).absoluteFilePath();
    processFrameAndUpdate();
}

void MainWindow::on_sourceSelect_currentIndexChanged(int index)
{
    mode = index;

    if(mode == WEBCAM)
    {
        ui->sourceDir->hide();
    }
    if(mode == IMAGE_FILE)
    {
        ui->sourceDir->show();
    }

    // Activate the timer to start processing
    qtimer->start();
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
   blur_on = arg1;

   // Activate the timer to start processing
   qtimer->start();
}
