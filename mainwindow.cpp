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
    resetWebcam();

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

    image_ext << "*.jpg" << "*.png";
    video_ext << "*.avi" << "*.mpg";
    fileModel->setNameFilters(image_ext);
    fileModel->setNameFilterDisables(false);

    QString sPath = QDir::currentPath();
    //QModelIndex idx = fileModel->setRootPath(sPath.append("/img3_ca"));
    QModelIndex idx = fileModel->setRootPath(sPath.append("/img3_ca"));

    QGridLayout* layout = new QGridLayout();
    sourceImage = new MediaBrowserQListView(this);
    QObject::connect(sourceImage, SIGNAL(clicked(QModelIndex)), this, SLOT(sourceImageChanged(QModelIndex)));     // Update after ocr done

    layout->addWidget(sourceImage);
    ui->listcontainer->setLayout(layout);
    sourceImage->setModel(fileModel);
    sourceImage->setRootIndex(idx);
    //sourceImage->hide();

    // Start with pre-processors off
    median_blur_on = 0;
    gauss_blur_on = 0;
    erode_dilate_on =0;
    // ui->sourceImage->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);

    run_tests = 0;

    setWindowTitle(tr("Rover"));

    // QProcess for running the tesseract OCR
    ocr = new QProcess(this);
    qDebug() <<QDir::currentPath();
    QObject::connect(ocr, SIGNAL(finished(int)), this, SLOT(updateInfo()));     // Update after ocr done

    // Set up timer to control update
    qtimer = new QTimer(this);
    connect(qtimer, SIGNAL(timeout()), this, SLOT(processFrameAndUpdate()));
    qtimer->setInterval(50);

    // Start in image mode
    mode = IMAGE_FILE;

    // Start the main process
    qtimer->start();

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

    // First check what mode we are to get the input

    // Check if we are geting input from webcam or file
    if( mode == WEBCAM)
    {
        capWebcam.read(matOriginal);
    }

    // If a video files is selected then play it
    else if( mode == VIDEO_FILE)
    {
        capVideo.read(matOriginal);
    }

    // Otherwise we have an image
    else if( mode == IMAGE_FILE )
    {
        matOriginal = cv::imread(img.toStdString());
    }

    if(matOriginal.empty() == true)
        return;

    cv::resize(matOriginal, matOriginal, cv::Size(640, 480));
    //cv::resize(matOriginal, matOriginal, cv::Size(320, 240));

    //cv::GaussianBlur(matOriginal, matProcessed, cv::Size(9,9), 1.5);

    // Make a copy of the matrix to process
    matOriginal.copyTo(matProcessed);

    // Perform pre processors that are selected
    if(median_blur_on)
        cv::medianBlur(matOriginal, matProcessed, 3);
    if(gauss_blur_on)
        cv::GaussianBlur(matProcessed, matProcessed, cv::Size(3,3), 1);

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

    // If we are only processing a static image then we can stop
    if(mode == IMAGE_FILE)
        qtimer->stop();

    run_tests=0;
}

// Method to reset the feed from the webcam
void MainWindow::resetWebcam()
{
    if(capWebcam.isOpened() == true)
        capWebcam.release();

    // Set up webcam
    capWebcam.open(CV_CAP_ANY);
    capWebcam.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    capWebcam.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

    // Connect to webcam
    if(capWebcam.isOpened() == false) {
        ui->info->appendPlainText("Error: cannot connect to webcam");
        return;
    }
}


cv::Mat MainWindow::drawHist(cv::Mat src, QString s){
IplImage temp = src;
IplImage* image = &temp;
CvHistogram* hist;
IplImage* imgHistogram = 0;

    //size of the histogram -1D histogram
         int bins = 50;
         int binWidth = 256 / bins;
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
         //     printf("min: %f, max: %f\n", min_value, max_value);

         //create an 8 bits single channel image to hold the histogram
         //paint it white
         imgHistogram = cvCreateImage(cvSize(bins * binWidth, 50),8,1);
         cvRectangle(imgHistogram, cvPoint(0,0), cvPoint(256,50), CV_RGB(255,255,255),-1);

         //draw the histogram :P
         for(int i=0; i < bins; i++){
                 value = cvQueryHistValue_1D( hist, i);
                 normalized = cvRound(value*50/max_value);
                 for (int k = 0 ; k < binWidth; k++){
                     cvLine(imgHistogram,cvPoint(i*binWidth + k,50), cvPoint(i*binWidth + k,50-normalized), CV_RGB(0,0,0));
                 }
         }


         //Calculating best threshold value
int cutoff = 2;
int max = 0;int max_at;
bestThreshold = 0;
         // Attemp two at threshold
// Find the first of the values responisble for the foreground
int i=0;
do {
    value = cvQueryHistValue_1D( hist, i);
    normalized = cvRound(value*50/max_value);
    i++;}
while (normalized < cutoff);
i--;
          for(; i < bins; i++){
                 value = cvQueryHistValue_1D( hist, i);
                 normalized = cvRound(value*50/max_value);
                 if(normalized < cutoff){
                    bestThreshold = i * binWidth;
                    break;
                 }
         }

bestThreshold = thresholdValue;
cvLine(imgHistogram,cvPoint(bestThreshold,0), cvPoint(bestThreshold,100), CV_RGB(200,50,75),3);

         return cv::Mat(imgHistogram);
//     cvShowImage(QString(s + "original").toStdString().c_str(), image );
//     cvShowImage(QString(s + "histogram").toStdString().c_str() , imgHistogram );

}

// This method will locate a possible plate in a image
//
// Input - cv::Mat image to be processed
// Output - cv::Mat image of the ROI
cv::Mat MainWindow::locatePlate(cv::Mat src) {

    // Rest old displayed images
    ui->histogramOriginal->clear();
    ui->histogramProcessed->clear();

    ui->plate->clear();
    ui->plateProcessed->clear();

    cv::Mat dest;
    cv::cvtColor(src, src, CV_RGB2BGR);
//    cv::cvtColor(src, dest, CV_BGR2GRAY);
dest = src.clone();

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
        thresholdValue = cv::threshold(temp, tempProcessed, bestThreshold + 20, 256, cv::THRESH_OTSU);

        cv::Mat histogramProcessed = drawHist(tempProcessed, "Post");

        // Display the histograms on the screen
        cv::cvtColor(histogram, histogram, CV_GRAY2RGB);
        cv::cvtColor(histogramProcessed, histogramProcessed, CV_GRAY2RGB);

        cv::cvtColor(temp, temp, CV_GRAY2RGB);
  //      cv::cvtColor(tempProcessed, tempProcessed, CV_GRAY2RGB);

/*
        // Start segmenting the letters - this will moved later
        // set up the parameters (check the defaults in opencv's code in blobdetector.cpp)
        cv::SimpleBlobDetector::Params params;
        params.minDistBetweenBlobs = 5.0f;
        params.filterByInertia = false;
        params.filterByConvexity = false;
        params.filterByColor = false;
        params.filterByCircularity = false;
        params.filterByArea = true;
        params.minArea = 10.0f;
        params.maxArea = 150.0f;
        // ... any other params you don't want default value

        // set up and create the detector using the parameters
        cv::Ptr<cv::FeatureDetector> blob_detector = new cv::SimpleBlobDetector(params);
        blob_detector->create("SimpleBlob");

        // detect!
        std::vector<cv::KeyPoint> keypoints;
        blob_detector->detect(tempProcessed, keypoints);

        cv::KeyPoint kp = keypoints.back();
        cv::drawKeypoints(tempProcessed, keypoints, tempProcessed);

       */
        // ***************** //


        /*
        //Linked list of connected pixel sequences in a binary image
        CvSeq* seq;

        //Array of bounding boxes
        std::vector<CvRect> boxes;

        //Memory allocated for OpenCV function operations
        CvMemStorage* storage = cvCreateMemStorage(0);
        cvClearMemStorage(storage);

        IplImage img_temp = (IplImage)tempProcessed;
        //Find connected pixel sequences within a binary OpenGL image (diff), starting at the top-left corner (0,0)
        cvFindContours(&img_temp, storage, &seq, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cvPoint(0,0));

        //Iterate through segments
        for(; seq; seq = seq->h_next) {
                //Find minimal bounding box for each sequence
            CvRect boundbox = cvBoundingRect(seq);
            boxes.push_back(boundbox);
        }

        for (int i =0; i < boxes.size(); i++)
            cv::rectangle(tempProcessed, boxes[i], 0);

  cv::cvtColor(temp, temp, CV_GRAY2RGB);
        cv::cvtColor(tempProcessed, tempProcessed, CV_GRAY2RGB);
*/
// ******************************************** //

        int thresh = 100;
        cv::Mat src_gray = tempProcessed;

        int erosion_size = 1;
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT,
                              cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1));//,
                              //cv::Point(erosion_size, erosion_size) );


        // Morphological erode and dilate
        if(erode_dilate_on){
            cv::dilate(src_gray, src_gray, cv::Mat());
            cv::erode(src_gray, src_gray, element);
         //   cv::dilate(src_gray, src_gray, element);
        }

        // Another attempt
        cv::Mat threshold_output;
        std::vector< std::vector<cv::Point> > contours;
        std::vector<cv::Vec4i> hierarchy;

        /// Detect edges using Threshold
        cv::threshold( src_gray, threshold_output, thresh, 255, cv::THRESH_BINARY );


         /// Find contours
         cv::findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

         /// Approximate contours to polygons + get bounding rects and circles
         std::vector< std::vector<cv::Point> > contours_poly( contours.size() );
         std::vector<cv::Rect> boundRect( contours.size() );
         std::vector<cv::Point2f>center( contours.size() );
         std::vector<float>radius( contours.size() );

         for( int i = 0; i < contours.size(); i++ )
            { cv::approxPolyDP( cv::Mat(contours[i]), contours_poly[i], 3, true );
              boundRect[i] = cv::boundingRect( cv::Mat(contours_poly[i]) );
              cv::minEnclosingCircle( contours_poly[i], center[i], radius[i] );
            }


         /// Draw polygonal contour + bonding rects + circles
         cv::Mat drawing = cv::Mat::zeros( threshold_output.size(), CV_8UC3 );
         cv::Rect text;
         cv::Point tl= cv::Point(drawing.cols / 2, drawing.rows /2 );
         cv::Point br = cv::Point(drawing.cols / 2, drawing.rows / 2);

         int char_width_min = src_gray.cols / 27;
         int char_width_max = src_gray.cols / 4;
         int char_height_min = src_gray.rows * (4 / 7.0);
         int char_height_max = src_gray.rows * (95 / 100.0);
         int max_area = src_gray.rows * src_gray.cols * (1/5.0);
         int min_area = src_gray.rows * src_gray.cols * (1/1000.0);

         // Prints out the min max heights of chars
       //  qDebug() << "charw:"<<char_width_min <<", charh:"<<char_height_min;
       //  qDebug() << "char_max:"<<max_area <<", char_min:"<<min_area;
       //  qDebug() << "conturs found:"<< contours.size();
         for( int i = 0; i< contours.size(); i++ )
            {
             // If the contour is the whole plate or smaller than a letter
             if( boundRect[i].area() > max_area || boundRect[i].area() < min_area)
                 continue;

             if( boundRect[i].width < char_width_min || boundRect[i].width > char_width_max)
                 continue;

             if( boundRect[i].height < char_height_min || boundRect[i].height > char_height_max)
                 continue;

             // Update the bounding box containg the text
             cv::Point new_tl = cv::Point(boundRect[i].tl());
             cv::Point new_br = cv::Point(boundRect[i].br());
             tl.x = new_tl.x < tl.x ? new_tl.x : tl.x;
             tl.y = new_tl.y < tl.y ? new_tl.y : tl.y;

             br.x = new_br.x > br.x ? new_br.x : br.x;
             br.y = new_br.y > br.y ? new_br.y : br.y;
             // ***************************************** //

             // cv::Scalar color = cv::Scalar ( cv::RNG.uniform(0, 255), cv::RNG.uniform(0,255), cv::RNG.uniform(0,255) );
              cv::Scalar color = cv::Scalar (0, 0, 0);

              // Draw contours on to drawing
              //cv::drawContours( drawing, contours_poly, i, color, 1, 8, std::vector< cv::Vec4i>(), 0, cv::Point() );

              //color = cv::Scalar (rand()&200 + 50, rand()&200 + 50, rand()&200 + 50);

              //coment out
              //cv::drawContours( drawing, contours_poly, i, color, 1, 8, std::vector< cv::Vec4i>(), 0, cv::Point() );

              cv::rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color, 1, 8, 0 );
           //   cv::circle( drawing, center[i], (int)radius[i], color, 2, 8, 0 );
            }

         for (int i =0;i < contours.size(); i++){
         //   cv::rectangle(tempProcessed, boundRect[i], 0);
            if ( boundRect[i].area() <  max_area && boundRect[i].area() > min_area)
            {
                char letter_name[20];
                sprintf(letter_name, "letter%d.jpg", i);
                cv::imwrite(letter_name, tempProcessed(boundRect[i]));
            }
         }
        cv::cvtColor(tempProcessed, tempProcessed, CV_GRAY2RGB);
        // Uncomment for contour drawing on image
       //  tempProcessed = drawing;

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

        text = cv::Rect(tl, br);
        // Write plate to file plate.jpg
        cv::imwrite("plate.jpg", tempProcessed(text));
        // Decode with tesseract
        ui->info->setPlainText("Reading plate...");
       // ocr->start("tesseract plate.jpg plate -psm 7 numberplate 2>&1 /dev/null");
        ocr->start("cuneiform -o plate.txt plate.jpg");

        cv::rectangle(dest, *r, cv::Scalar(255, 50, 50),3);
    }

    // Swap back temp so we avoid the rectangles we draw in the processing
    cv::cvtColor(dest, dest, CV_BGR2RGB);
    return dest;
}


void MainWindow::updateInfo()
{
    // Read text file plate.txt and display
    QFile file("plate.txt");
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    QString text = stream.readLine();
    if(text.trimmed() != "")
    {
        text.remove(QRegExp("[^a-zA-Z\\d\]"));
        ui->info->setPlainText(text);
    }
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

void MainWindow::on_sourceSelect_currentIndexChanged(int index)
{
    mode = index;

    if(mode == WEBCAM)
    {
        //resetWebcam();
        sourceImage->hide();
    }
    if(mode == IMAGE_FILE)
    {
        fileModel->setNameFilters(image_ext);
        sourceImage->show();
    }

    if(mode == VIDEO_FILE)
    {
        capVideo.release();     // Release any previous video file
        fileModel->setNameFilters(video_ext);
        sourceImage->show();
    }

    // Activate the timer to start processing
    qtimer->start();
}


void MainWindow::sourceImageChanged(const QModelIndex &index)
{
    // Get the image or video
    img = fileModel->fileInfo(index).absoluteFilePath();

    // If the source is a video file
    if(mode == VIDEO_FILE)
    {
        capVideo.open(img.toStdString());
    qtimer->start();
    return;
    }

   processFrameAndUpdate();
}



void MainWindow::on_medianBlurCheckbox_stateChanged(int arg1)
{
   median_blur_on = arg1;

   // Activate the timer to start processing
   qtimer->start();

}

void MainWindow::on_gaussianBlurCheckbox_stateChanged(int arg1)
{

   gauss_blur_on = arg1;

   // Activate the timer to start processing
   qtimer->start();
}

void MainWindow::on_erodeDilateCheckbox_stateChanged(int arg1)
{
    erode_dilate_on = arg1;

    // Activate the timer to start processing
    qtimer->start();
}

void MainWindow::on_runTestsButon_clicked()
{
    run_tests = 1;
    // Start running tests on all files in this folder
    QString sPath = QDir::currentPath();
    QDir myDir(sPath.append("/img3_ca"));

int count =0;
    QFileInfoList list = myDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    foreach(QFileInfo finfo, list) {
        img =  finfo.absoluteFilePath();
        processFrameAndUpdate();
        ocr->waitForFinished();

        QString detected = ui->info->toPlainText();
        detected.toUpper();
        // Just to be safe remove any extra chars that may have been added
        detected.remove(QRegExp("[^a-zA-Z\\d\]"));
        qDebug() << "Detected:" + detected;
        qDebug() << "Actual:" + finfo.baseName();
        if(finfo.baseName() == detected){
            count++;
            qDebug() << "hit!";
        }
        else
            qDebug() << "miss";
        }

        // Check if the result was correct
        qDebug() << "Finished!" << "Total hits:" << count;

}
