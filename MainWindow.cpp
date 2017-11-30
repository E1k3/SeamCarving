#include "MainWindow.hpp"

#include "cv_utility.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    /* Initialisiere die UI Komponenten */
    setupUi();
}

MainWindow::~MainWindow()
{
    /* loesche die UI Komponenten */
    delete centralWidget;    
    
    /* schliesse alle offenen Fenster */
    cv::destroyAllWindows();
}

/* Methode oeffnet ein Bild und zeigt es in einem separaten Fenster an */
void MainWindow::on_pbOpenImage_clicked()
{
    /* oeffne Bild mit Hilfe eines Dateidialogs */
	QString imagePath = QFileDialog::getOpenFileName(this, "Open Image...", QString(), QString("Images *.png *.jpg *.tiff *.tif *.jpeg"));
    
    /* wenn ein gueltiger Dateipfad angegeben worden ist... */
    if(!imagePath.isNull() && !imagePath.isEmpty())
    {
        /* ...lese das Bild ein */
        cv::Mat img = cv::imread(QtOpencvCore::qstr2str(imagePath));
        
        /* wenn das Bild erfolgreich eingelesen worden ist... */
        if(!img.empty())
        {
            /* ...merke das Originalbild... */
            originalImage = img;
            
            /* ...aktiviere das UI... */
            enableGUI();
            
            /* ...zeige das Originalbild in einem separaten Fenster an */
			cv::namedWindow("Original Image", cv::WINDOW_FREERATIO);
            cv::imshow("Original Image", originalImage); 

			sbRows->setMinimum(0);
			sbRows->setMaximum(originalImage.rows-2);
			sbCols->setMinimum(0);
			sbCols->setMaximum(originalImage.cols-2);
		}
        else
        {
            /* ...sonst deaktiviere das UI */
            disableGUI();
        }
    }
}

void MainWindow::on_pbComputeSeams_clicked()
{
    /* Anzahl der Spalten, die entfernt werden sollen */
    int colsToRemove = sbCols->value();
    
    /* Anzahl der Zeilen, die entfernt werden sollen */
    int rowsToRemove = sbRows->value();
    
    /* .............. */
	gray = cvutil::grayscale(originalImage);

	vertical_seams.clear();
	vertical_seams.reserve(static_cast<size_t>(colsToRemove));
	auto real_seam = std::vector<int>{};
	auto original_copy = originalImage.clone();

	for(int c = 0; c < colsToRemove; ++c)
	{
		energy = cvutil::energy(gray);
		vertical_seams.push_back(cvutil::vertical_seam(energy));
		cvutil::remove_vertical_seam<uchar>(gray, vertical_seams.back());

		// Mark found seams
		if(cbMark->isChecked())
		{
			real_seam = vertical_seams.back();
			for(int r = 0; r < energy.rows; ++r)
			{
				for(int s = static_cast<int>(vertical_seams.size())-2; s >= 0; --s)
					if(vertical_seams[static_cast<size_t>(s)][static_cast<size_t>(r)] <= real_seam[static_cast<size_t>(r)])
						++real_seam[static_cast<size_t>(r)];
				original_copy.at<cv::Vec<uchar, 3>>(r, real_seam[static_cast<size_t>(r)])[0] = 255;
				original_copy.at<cv::Vec<uchar, 3>>(r, real_seam[static_cast<size_t>(r)])[1] = 0;
				original_copy.at<cv::Vec<uchar, 3>>(r, real_seam[static_cast<size_t>(r)])[2] = 0;
			}
			cv::imshow("Original Image", original_copy);
		}
	}
	horizontal_seams.clear();
	horizontal_seams.reserve(static_cast<size_t>(rowsToRemove));
	auto real_horizontal_seams = std::vector<std::vector<int>>{};
	real_horizontal_seams.reserve(static_cast<size_t>(rowsToRemove));

	for(int r = 0; r < rowsToRemove; ++r)
	{
		energy = cvutil::energy(gray);
		horizontal_seams.push_back(cvutil::horizontal_seam(energy));
		cvutil::remove_horizontal_seam<uchar>(gray, horizontal_seams.back());

		// Mark found seams (not completely accurate)
		if(cbMark->isChecked())
		{
			real_seam = horizontal_seams.back();
			for(int c = 0; c < energy.cols; ++c)
			{
				for(int s = static_cast<int>(horizontal_seams.size())-2; s >= 0; --s)
					if(horizontal_seams[static_cast<size_t>(s)][static_cast<size_t>(c)] <= real_seam[static_cast<size_t>(c)])
						++real_seam[static_cast<size_t>(c)];
				int c_real = c;
				for(int s = static_cast<int>(vertical_seams.size())-2; s >= 0; --s)
					if(vertical_seams[static_cast<size_t>(s)][static_cast<size_t>(real_seam[static_cast<size_t>(c)])] <= c_real)
						++c_real;

				original_copy.at<cv::Vec<uchar, 3>>(real_seam[static_cast<size_t>(c)], c_real)[0] = 0;
				original_copy.at<cv::Vec<uchar, 3>>(real_seam[static_cast<size_t>(c)], c_real)[1] = 0;
				original_copy.at<cv::Vec<uchar, 3>>(real_seam[static_cast<size_t>(c)], c_real)[2] = 255;
			}
			cv::imshow("Original Image", original_copy);
		}
	}
}

void MainWindow::on_pbRemoveSeams_clicked()
{
	carved = originalImage.clone();
	for(const auto& seam : vertical_seams)
		cvutil::remove_vertical_seam<cv::Vec<uchar, 3>>(carved, seam);
	vertical_seams.clear();

	for(const auto& seam : horizontal_seams)
		cvutil::remove_horizontal_seam<cv::Vec<uchar, 3>>(carved, seam);
	horizontal_seams.clear();

	cv::namedWindow("Carved Image", cv::WINDOW_GUI_EXPANDED);
	cv::imshow("Carved Image", carved);
}

void MainWindow::setupUi()
{
    /* Boilerplate code */
    /*********************************************************************************************/
    resize(220, 250);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(220, 250));
    setMaximumSize(QSize(220, 250));
    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QString("centralWidget"));
    
    horizontalLayout = new QHBoxLayout(centralWidget);
    verticalLayout = new QVBoxLayout();
    
    pbOpenImage = new QPushButton(QString("Open Image"), centralWidget);
    verticalLayout->addWidget(pbOpenImage);
    
    
    verticalLayout_3 = new QVBoxLayout();
    lCaption = new QLabel(QString("Remove"), centralWidget);
    lCaption->setEnabled(false);
    verticalLayout_3->addWidget(lCaption);
    
    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setObjectName(QString("horizontalLayout_3"));
    lCols = new QLabel(QString("Cols"), centralWidget);
    lCols->setEnabled(false);
    lRows = new QLabel(QString("Rows"), centralWidget);
    lRows->setEnabled(false);
    horizontalLayout_3->addWidget(lCols);
    horizontalLayout_3->addWidget(lRows);
    verticalLayout_3->addLayout(horizontalLayout_3);
    
    horizontalLayout_2 = new QHBoxLayout();
    sbCols = new QSpinBox(centralWidget);
    sbCols->setEnabled(false);
    horizontalLayout_2->addWidget(sbCols);
    sbRows = new QSpinBox(centralWidget);
    sbRows->setEnabled(false);
    horizontalLayout_2->addWidget(sbRows);
    verticalLayout_3->addLayout(horizontalLayout_2);
    verticalLayout->addLayout(verticalLayout_3);
    
    pbComputeSeams = new QPushButton(QString("Compute Seams"), centralWidget);
    pbComputeSeams->setEnabled(false);
    verticalLayout->addWidget(pbComputeSeams);
    
    pbRemoveSeams = new QPushButton(QString("Remove Seams"), centralWidget);
    pbRemoveSeams->setEnabled(false);
    verticalLayout->addWidget(pbRemoveSeams);

	cbMark = new QCheckBox(QString("Mark seams (experimental)"), centralWidget);
	cbMark->setEnabled(false);
	verticalLayout->addWidget(cbMark);

    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout->addItem(verticalSpacer);
    horizontalLayout->addLayout(verticalLayout);
    
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);
    setCentralWidget(centralWidget);
    /*********************************************************************************************/
    
    
    /* Verbindung zwischen den Buttonklicks und den Methoden, die beim jeweiligen Buttonklick ausgefuehrt werden sollen */
    connect(pbOpenImage,    &QPushButton::clicked, this, &MainWindow::on_pbOpenImage_clicked);  
    connect(pbComputeSeams, &QPushButton::clicked, this, &MainWindow::on_pbComputeSeams_clicked); 
    connect(pbRemoveSeams,  &QPushButton::clicked, this, &MainWindow::on_pbRemoveSeams_clicked);
}

void MainWindow::enableGUI()
{
    lCaption->setEnabled(true);
    
    lCols->setEnabled(true);
    lRows->setEnabled(true);
    
    sbCols->setEnabled(true);
    sbRows->setEnabled(true);
    
    pbComputeSeams->setEnabled(true);
    pbRemoveSeams->setEnabled(true);

	cbMark->setEnabled(true);
    
    sbRows->setMinimum(0);
    sbRows->setMaximum(originalImage.rows);
    sbRows->setValue(2);
    
    sbCols->setMinimum(0);
    sbCols->setMaximum(originalImage.cols);
    sbCols->setValue(2);
}

void MainWindow::disableGUI()
{
    lCaption->setEnabled(false);
    
    lCols->setEnabled(false);
    lRows->setEnabled(false);
    
    sbCols->setEnabled(false);
    sbRows->setEnabled(false);
    
    pbComputeSeams->setEnabled(false);
    pbRemoveSeams->setEnabled(false);

	cbMark->setEnabled(false);
}
