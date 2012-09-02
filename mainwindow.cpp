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
    QModelIndex idx = fileModel->setRootPath(sPath);

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
    cv::medianBlur(matOriginal, matProcessed, 9);

    cv::cvtColor(matOriginal, matOriginal, CV_BGR2RGB);
    cv::cvtColor(matProcessed, matProcessed, CV_BGR2RGB);

    QImage qimgOriginal((uchar*)matOriginal.data, matOriginal.cols, matOriginal.rows, matOriginal.step, QImage::Format_RGB888);
    QImage qimgProcessed((uchar*)matProcessed.data, matProcessed.cols, matProcessed.rows, matProcessed.step, QImage::Format_RGB888);

    ui->original->setPixmap(QPixmap::fromImage(qimgOriginal));
    ui->processed->setPixmap(QPixmap::fromImage(qimgProcessed));

    if(mode == IMAGE_FILE)
        qtimer->stop();

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
