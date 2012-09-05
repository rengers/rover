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

    ui->sourceImage->setModel(fileModel);
    ui->sourceImage->setRootIndex(idx);
    ui->sourceImage->hide();

   // ui->sourceImage->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);

    setWindowTitle(tr("Rover"));

    // QProcess for running the tesseract OCR
    ocr = new QProcess(this);
    qDebug() <<QDir::currentPath();
    QObject::connect(ocr, SIGNAL(finished(int)), this, SLOT(updateInfo()));     // Update after ocr done

    // Set up timer to control update
    qtimer = new QTimer(this);
    connect(qtimer, SIGNAL(timeout()), this, SLOT(processFrameAndUpdate()));
    qtimer->start(500);

    //QObject::connect(qtimer, SIGNAL(timeout()), this, SLOT(updateInfo()));
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

    ui->original->setPixmap(QPixmap::fromImage(qimgOriginal));
    ui->processed->setPixmap(QPixmap::fromImage(qimgProcessed));

    if(mode == IMAGE_FILE)
        qtimer->stop();

}

cv::Mat drawHist(cv::Mat src, QString s){
IplImage temp = src;
IplImage* image = &temp;
CvHistogram* hist;
IplImage* imgHistogram = 0;

    //size of the histogram -1D histogram
         int bins = 256;
         int hsize[] = {bins};

         //max and min value of the histogram
         float max_value = 0, min_value = 0;

         //value and normalized value
         float value;
         int normalized;

         //ranges - grayscale 0 to 256
         float xranges[] = { 0, 256 };
         float* ranges[] = { xranges };

         //create an 8 bit single channel image to hold a
         //grayscale version of the original picture
         IplImage* gray = cvCreateImage( cvGetSize(image), 8, 1 );
       //  cvCvtColor( image, gray, CV_BGR2GRAY );
         cvCopy(image, gray);

         //planes to obtain the histogram, in this case just one
         IplImage* planes[] = { gray };

         //get the histogram and some info about it
         hist = cvCreateHist( 1, hsize, CV_HIST_ARRAY, ranges,1);
         cvCalcHist( planes, hist, 0, NULL);
         cvGetMinMaxHistValue( hist, &min_value, &max_value);
         printf("min: %f, max: %f\n", min_value, max_value);

         //create an 8 bits single channel image to hold the histogram
         //paint it white
         imgHistogram = cvCreateImage(cvSize(bins, 50),8,1);
         cvRectangle(imgHistogram, cvPoint(0,0), cvPoint(256,50), CV_RGB(255,255,255),-1);

         //draw the histogram :P
         for(int i=0; i < bins; i++){
                 value = cvQueryHistValue_1D( hist, i);
                 normalized = cvRound(value*50/max_value);
                 cvLine(imgHistogram,cvPoint(i,50), cvPoint(i,50-normalized), CV_RGB(0,0,0));
         }


         //Calculating best threshold value
int cutoff = 10;
int max = 0;int max_at;
int bestThreshold=0;
         for(int i=0; i < bins; i++){
                 value = cvQueryHistValue_1D( hist, i);
                 normalized = cvRound(value*50/max_value);
                 if(normalized > max){
                    max = normalized;
                            max_at = i;
                 }
         }

         for(int i=max_at; i < bins; i++){
                 value = cvQueryHistValue_1D( hist, i);
                 normalized = cvRound(value*50/max_value);
                 if(normalized < cutoff){
                    bestThreshold = i;
                    break;
                 }
         }
//cvLine(imgHistogram,cvPoint(bestThreshold,50), cvPoint(bestThreshold,50-max), CV_RGB(200,50,75),3);

         return cv::Mat(imgHistogram);
//     cvShowImage(QString(s + "original").toStdString().c_str(), image );
//     cvShowImage(QString(s + "histogram").toStdString().c_str() , imgHistogram );

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

        cv::Mat temp;
        cv::Mat tempProcessed;
        cv::cvtColor(src(*r), temp, CV_BGR2GRAY);
        cv::cvtColor(src(*r), temp, CV_BGR2GRAY);

        cv::Mat histogram = drawHist(temp, "Pre");
        cv::threshold(temp, tempProcessed, 80, 255, 0);
        cv::Mat histogramProcessed = drawHist(tempProcessed, "Post");

        // Display the histograms on the screen
        cv::cvtColor(histogram, histogram, CV_GRAY2RGB);
        cv::cvtColor(histogramProcessed, histogramProcessed, CV_GRAY2RGB);

        cv::cvtColor(temp, temp, CV_GRAY2RGB);
        cv::cvtColor(tempProcessed, tempProcessed, CV_GRAY2RGB);

        QImage qimgHist((uchar*)histogram.data, histogram.cols, histogram.rows, histogram.step, QImage::Format_RGB888);
        QImage qimgHistProcessed((uchar*)histogramProcessed.data, histogramProcessed.cols, histogramProcessed.rows, histogramProcessed.step, QImage::Format_RGB888);
        QImage qimgPlate((uchar*)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
        QImage qimgPlateProcessed((uchar*)tempProcessed.data, tempProcessed.cols, tempProcessed.rows, tempProcessed.step, QImage::Format_RGB888);



        //qimgHist = qimgHist.scaledToWidth(320);
        //qimgHistProcessed = qimgHistProcessed.scaledToWidth(320);

        ui->histogramOriginal->setPixmap(QPixmap::fromImage(qimgHist));
        ui->histogramProcessed->setPixmap(QPixmap::fromImage(qimgHistProcessed));

        ui->plate->setPixmap(QPixmap::fromImage(qimgPlate));
        ui->plateProcessed->setPixmap(QPixmap::fromImage(qimgPlateProcessed));

        // *************************************************************8


        // Write plate to file plate.jpg
        cv::imwrite("plate.jpg", tempProcessed);
        // Decode with tesseract
        ui->info->setPlainText("Reading plate...");
        ocr->start("tesseract plate.jpg plate -psm 3 nobatch numberplates 2>&1 /dev/null");

        cv::rectangle(src, *r, cv::Scalar(255, 50, 50),3);
    }


    cv::cvtColor(src, src, CV_BGR2RGB);
    return src;
}


void MainWindow::updateInfo()
{
    // Read text file plate.txt and display
    QFile file("plate.txt");
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    QString text = stream.readLine();
    if(text.trimmed() != "")
        ui->info->setPlainText(text);
    else
        ui->info->setPlainText("Could not read plate");
    file.close();

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

void MainWindow::on_sourceImage_clicked(const QModelIndex &index)
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
        ui->sourceImage->hide();
    }
    if(mode == IMAGE_FILE)
    {
        ui->sourceImage->show();
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

void MainWindow::on_sourceImage_activated(const QModelIndex &index)
{
    // Update image
    img = fileModel->fileInfo(index).absoluteFilePath();
    processFrameAndUpdate();
}

class MediaBrowserQListView : public QListView
{
public:
    MediaBrowserQListView(QWidget * parent) : QListView(parent) {}
protected:
    void keyPressEvent(QKeyEvent *event)
    {
        QModelIndex oldIdx = currentIndex();
        QListView::keyPressEvent(event);
        QModelIndex newIdx = currentIndex();
        if(oldIdx.row() != newIdx.row())
        {
            emit clicked(newIdx);
        }
    }
};
