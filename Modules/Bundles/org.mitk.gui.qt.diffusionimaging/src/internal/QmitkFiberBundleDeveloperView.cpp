/*=========================================================================
 
 Program:   Medical Imaging & Interaction Toolkit
 Language:  C++
 Date:      $Date: 2010-03-31 16:40:27 +0200 (Mi, 31 Mrz 2010) $
 Version:   $Revision: 21975 $
 
 Copyright (c) German Cancer Research Center, Division of Medical and
 Biological Informatics. All rights reserved.
 See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.
 
 =========================================================================*/
//=========FOR TESTING==========
//random generation, number of points equal requested points



// Blueberry application and interaction service
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "QmitkFiberBundleDeveloperView.h"
#include <QmitkStdMultiWidget.h> 

// Qt
#include <QTimer>

// MITK
//#include <mitkFiberBundleX.h> //for fiberStructure

//===needed when timeSlicedGeometry is null to invoke rendering mechansims ====
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>


// VTK
#include <vtkPointSource.h> //for randomized FiberStructure
#include <vtkPolyLine.h>  //for fiberStructure
#include <vtkCellArray.h> //for fiberStructure
#include <vtkMatrix4x4.h> //for geometry

//ITK
#include <itkTimeProbe.h>

//==============================================
//======== W O R K E R S ____ S T A R T ========
//==============================================
/*===================================================================================
 * THIS METHOD IMPLEMENTS THE ACTIONS WHICH SHALL BE EXECUTED by the according THREAD
 * --generate FiberIDs--*/
QmitkFiberIDWorker::QmitkFiberIDWorker(QThread* hostingThread, Package4WorkingThread itemPackage)
: m_itemPackage(itemPackage),
m_hostingThread(hostingThread)
{
  
}
void QmitkFiberIDWorker::run()
{
  
  /* MEASUREMENTS AND FANCY GUI EFFECTS
   * accurate time measurement using ITK timeProbe*/
  itk::TimeProbe clock;
  clock.Start();
  //set GUI representation of timer to 0, is essential for correct timer incrementation
  m_itemPackage.st_Controls->infoTimerGenerateFiberIds->setText(QString::number(0)); 
  m_itemPackage.st_FancyGUITimer1->start();
  
  //do processing
  m_itemPackage.st_FBX->DoGenerateFiberIds();
  
  /* MEASUREMENTS AND FANCY GUI EFFECTS CLEANUP */
  clock.Stop();
  m_itemPackage.st_FancyGUITimer1->stop();
  m_itemPackage.st_Controls->infoTimerGenerateFiberIds->setText( QString::number(clock.GetTotal()) );
  delete m_itemPackage.st_FancyGUITimer1; // fancy timer is not needed anymore
  m_hostingThread->quit();
  
}


/*===================================================================================
 * THIS METHOD IMPLEMENTS THE ACTIONS WHICH SHALL BE EXECUTED by the according THREAD
 * --generate random fibers--*/
QmitkFiberGenerateRandomWorker::QmitkFiberGenerateRandomWorker(QThread* hostingThread, Package4WorkingThread itemPackage)
: m_itemPackage(itemPackage),
m_hostingThread(hostingThread)
{
  
}
void QmitkFiberGenerateRandomWorker::run()
{
  
  /* MEASUREMENTS AND FANCY GUI EFFECTS */
  m_itemPackage.st_Controls->infoTimerGenerateFiberBundle->setText(QString::number(0)); 
  m_itemPackage.st_FancyGUITimer1->start();
  
  //do processing, generateRandomFibers
  int numOfFibers = m_itemPackage.st_Controls->boxFiberNumbers->value();
  int distrRadius = m_itemPackage.st_Controls->boxDistributionRadius->value();
  int numOfPoints = numOfFibers * distrRadius;
  
  std::vector< std::vector<int> > fiberStorage;
  for (int i=0; i<numOfFibers; ++i) {
    std::vector<int> a;
    fiberStorage.push_back( a );
  }
  
  /* Generate Point Cloud */
  vtkSmartPointer<vtkPointSource> randomPoints = vtkSmartPointer<vtkPointSource>::New();
  randomPoints->SetCenter(0.0, 0.0, 0.0);
  randomPoints->SetNumberOfPoints(numOfPoints);
  randomPoints->SetRadius(distrRadius);
  randomPoints->Update();
  vtkPoints* pnts = randomPoints->GetOutput()->GetPoints();
  
  /* ASSIGN EACH POINT TO A RANDOM FIBER */
  srand((unsigned)time(0)); // init randomizer
  for (int i=0; i<pnts->GetNumberOfPoints(); ++i) {
    
    //generate random number between 0 and numOfFibers-1
    int random_integer; 
    random_integer = (rand()%numOfFibers); 
    
    //add current point to random fiber
    fiberStorage.at(random_integer).push_back(i); 
    //    MITK_INFO << "point" << i << " |" << pnts->GetPoint(random_integer)[0] << "|" << pnts->GetPoint(random_integer)[1]<< "|" << pnts->GetPoint(random_integer)[2] << "| into fiber" << random_integer;
  } 
  
  // initialize accurate time measurement
  itk::TimeProbe clock;
  clock.Start();
  
  /* GENERATE VTK POLYLINES OUT OF FIBERSTORAGE */
  vtkSmartPointer<vtkCellArray> linesCell = vtkSmartPointer<vtkCellArray>::New(); // Host vtkPolyLines
  linesCell->Allocate(pnts->GetNumberOfPoints()*2); //allocate for each cellindex also space for the pointId, e.g. [idx | pntID]
  for (unsigned long i=0; i<fiberStorage.size(); ++i)
  {
    std::vector<int> singleFiber = fiberStorage.at(i);
    vtkSmartPointer<vtkPolyLine> fiber = vtkSmartPointer<vtkPolyLine>::New();
    fiber->GetPointIds()->SetNumberOfIds((int)singleFiber.size());
    
    for (unsigned long si=0; si<singleFiber.size(); ++si) 
    {  //hopefully unsigned long to double works fine in VTK ;-)
      fiber->GetPointIds()->SetId( si, singleFiber.at(si) );
    }
    
    linesCell->InsertNextCell(fiber);
  }  
  
  /* checkpoint for cellarray allocation */
  if ( (linesCell->GetSize()/pnts->GetNumberOfPoints()) != 2 ) //e.g. size: 12, number of points:6 .... each cell hosts point ids (6 ids) + cell index for each idPoint. 6 * 2 = 12
  {
    MITK_INFO << "RANDOM FIBER ALLOCATION CAN NOT BE TRUSTED ANYMORE! Correct leak or remove command: linesCell->Allocate(pnts->GetNumberOfPoints()*2) but be aware of possible loss in performance.";
  }
  
  /* HOSTING POLYDATA FOR RANDOM FIBERSTRUCTURE */
  vtkPolyData* PDRandom = vtkPolyData::New(); //no need to delete because data is needed in datastorage.
  PDRandom->SetPoints(pnts);
  PDRandom->SetLines(linesCell);
  
  // accurate timer measurement stop
  clock.Stop();
  //MITK_INFO << "=====Assambling random Fibers to Polydata======\nMean: " << clock.GetMean() << " Total: " << clock.GetTotal() << std::endl;  
  
  // call function to convert fiberstructure into fiberbundleX and pass it to datastorage
  (m_itemPackage.st_host->*m_itemPackage.st_pntr_to_Method_PutFibersToDataStorage)(PDRandom);
  
  /* MEASUREMENTS AND FANCY GUI EFFECTS CLEANUP */
  m_itemPackage.st_FancyGUITimer1->stop();
  m_itemPackage.st_Controls->infoTimerGenerateFiberBundle->setText( QString::number(clock.GetTotal()) );
  delete m_itemPackage.st_FancyGUITimer1; // fancy timer is not needed anymore
  m_hostingThread->quit();
  
}


/*===================================================================================
 * THIS METHOD IMPLEMENTS THE ACTIONS WHICH SHALL BE EXECUTED by the according THREAD
 * --update GUI elements of thread monitor--
 * implementation not thread safe, not needed so far because
 * there exists only 1 thread for fiberprocessing
 * for threadsafety, you need to implement checking mechanisms in methods "::threadFor...." */
QmitkFiberThreadMonitorWorker::QmitkFiberThreadMonitorWorker( QThread* hostingThread, Package4WorkingThread itemPackage )
: m_itemPackage(itemPackage),
m_hostingThread(hostingThread)
{

  
  //set timers
  m_thtimer_threadStarted  = new QTimer;
  m_thtimer_threadStarted->setInterval(100);
  
  m_thtimer_initMonitor = new QTimer;
  m_thtimer_initMonitor->setInterval(100);
  
  connect (m_thtimer_threadStarted, SIGNAL( timeout()), this, SLOT( fancyTextFading_threadStarted() ) );
  connect (m_thtimer_initMonitor, SIGNAL( timeout()), this, SLOT( fancyMonitorInitialization() ) );
  
  //first, the current text shall turn transparent
  m_decreaseOpacity_threadStarted = true;
  
}
void QmitkFiberThreadMonitorWorker::run()
{
  
}

void QmitkFiberThreadMonitorWorker::threadForFiberProcessingStarted()
{
  MITK_INFO << "...thread initialized...";
  
  m_thtimer_threadStarted->start();
  
}

void QmitkFiberThreadMonitorWorker::initializeMonitor()
{
  MITK_INFO << "...Pfiantench liabe leidln...";

  m_thtimer_initMonitor->start();
}

void QmitkFiberThreadMonitorWorker::fancyTextFading_threadStarted()
{
  MITK_INFO << "...----RUNRUNRUN----...";  
  
  if (m_decreaseOpacity_threadStarted) {

    /*int currentOpacity = QString.toInt( */ m_itemPackage.st_ThreadMonitorDataNode; /* ->getCurrentOpacityValue() */
    m_itemPackage.st_ThreadMonitorDataNode; /* ->SetCuerrentOpacity( --currentOpacity )*/
    m_itemPackage.st_ThreadMonitorDataNode->Modified();
//    m_itemPackage.st_MultiWidget->RequestUpdate(rendererOfRenWin4);
    
    if (true /* DN->current_opacity == 0.0 */) {
      m_decreaseOpacity_threadStarted = false;
    }
    
  } else {
    
    
    /*int currentOpacity = QString.toInt( */ m_itemPackage.st_ThreadMonitorDataNode; /* ->getCurrentOpacityValue() */
    m_itemPackage.st_ThreadMonitorDataNode; /* ->SetCuerrentOpacity( ++currentOpacity )*/
    m_itemPackage.st_ThreadMonitorDataNode->Modified();
    //    m_itemPackage.st_MultiWidget->RequestUpdate(rendererOfRenWin4);
    
    if (true /* GET CURRENT OPACITY VALUE == DN->getDesiredOpacityValue() */) {
      m_thtimer_threadStarted->stop();
    }

    
  }
  

}

void QmitkFiberThreadMonitorWorker::fancyMonitorInitialization()
{
  MITK_INFO << "...----Byebye----...";
  mitk::Point2D pntClose = m_itemPackage.st_FBX_Monitor->getBracketClosePosition();
  m_thtimer_initMonitor->stop();
}
//==============================================
//======== W O R K E R S ________ E N D ========
//==============================================




// ========= HERE STARTS THE ACTUAL FIBERB2UNDLE DEVELOPER VIEW IMPLEMENTATION ======
const std::string QmitkFiberBundleDeveloperView::VIEW_ID = "org.mitk.views.fiberbundledeveloper";
const std::string id_DataManager = "org.mitk.views.datamanager";
using namespace berry;


QmitkFiberBundleDeveloperView::QmitkFiberBundleDeveloperView()
: QmitkFunctionality()
, m_Controls( 0 )
, m_MultiWidget( NULL )
{
  m_hostThread = new QThread;
  m_threadInProgress = false;
  
}

// Destructor
QmitkFiberBundleDeveloperView::~QmitkFiberBundleDeveloperView()
{
  //m_FiberBundleX->Delete(); using weakPointer, therefore no delete necessary
  delete m_hostThread;
  if (m_FiberIDGenerator != NULL)
    delete m_FiberIDGenerator;
  
  if (m_GeneratorFibersRandom != NULL)
    delete m_GeneratorFibersRandom;
  
}


void QmitkFiberBundleDeveloperView::CreateQtPartControl( QWidget *parent )
{
  // build up qt view, unless already done in QtDesigner, etc.
  if ( !m_Controls )
  {
    // create GUI widgets from the Qt Designer's .ui file
    m_Controls = new Ui::QmitkFiberBundleDeveloperViewControls;
    m_Controls->setupUi( parent );
    
    /*=========INITIALIZE BUTTON CONFIGURATION ================*/
    m_Controls->radioButton_directionX->setEnabled(false);
    m_Controls->radioButton_directionY->setEnabled(false);
    m_Controls->radioButton_directionZ->setEnabled(false);
    m_Controls->buttonGenerateFiberIds->setEnabled(false);
    m_Controls->buttonGenerateFibers->setEnabled(true);
    
    m_Controls->buttonColorFibers->setEnabled(false);//not yet implemented
    m_Controls->buttonSMFibers->setEnabled(false);//not yet implemented
    m_Controls->buttonVtkDecimatePro->setEnabled(false);//not yet implemented
    m_Controls->buttonVtkSmoothPD->setEnabled(false);//not yet implemented
    m_Controls->buttonGenerateTubes->setEnabled(false);//not yet implemented
    
    
    connect( m_Controls->buttonGenerateFibers, SIGNAL(clicked()), this, SLOT(DoGenerateFibers()) );
    connect( m_Controls->buttonGenerateFiberIds, SIGNAL(pressed()), this, SLOT(DoGenerateFiberIDs()) );
    
    connect( m_Controls->radioButton_directionRandom, SIGNAL(clicked()), this, SLOT(DoUpdateGenerateFibersWidget()) );
    connect( m_Controls->radioButton_directionX, SIGNAL(clicked()), this, SLOT(DoUpdateGenerateFibersWidget()) );
    connect( m_Controls->radioButton_directionY, SIGNAL(clicked()), this, SLOT(DoUpdateGenerateFibersWidget()) );
    connect( m_Controls->radioButton_directionZ, SIGNAL(clicked()), this, SLOT(DoUpdateGenerateFibersWidget()) );
    connect( m_Controls->toolBox, SIGNAL(currentChanged ( int ) ), this, SLOT(SelectionChangedToolBox(int)) );
    
    connect( m_Controls->checkBoxMonitorFiberThreads, SIGNAL(stateChanged(int)), this, SLOT(DoMonitorFiberThreads(int)) );
    
    
  }
  
  //  Checkpoint for fiber ORIENTATION
  if ( m_DirectionRadios.empty() )
  {
    m_DirectionRadios.insert(0, m_Controls->radioButton_directionRandom);
    m_DirectionRadios.insert(1, m_Controls->radioButton_directionX);
    m_DirectionRadios.insert(2, m_Controls->radioButton_directionY);
    m_DirectionRadios.insert(3, m_Controls->radioButton_directionZ);
  }
  
  // set GUI elements of FiberGenerator to according configuration
  DoUpdateGenerateFibersWidget();
  
  
}
/* THIS METHOD UPDATES ALL GUI ELEMENTS OF QGroupBox DEPENDING ON CURRENTLY SELECTED
 * RADIO BUTTONS 
 */
void QmitkFiberBundleDeveloperView::DoUpdateGenerateFibersWidget()
{
  //get selected radioButton
  QString fibDirection; //stores the object_name of selected radiobutton 
  QVector<QRadioButton*>::const_iterator i;
  for (i = m_DirectionRadios.begin(); i != m_DirectionRadios.end(); ++i) 
  {
    QRadioButton* rdbtn = *i;
    if (rdbtn->isChecked())
      fibDirection = rdbtn->objectName();
  }
  
  if ( fibDirection == FIB_RADIOBUTTON_DIRECTION_RANDOM ) {
    // disable radiobuttons
    if (m_Controls->boxFiberMinLength->isEnabled())
      m_Controls->boxFiberMinLength->setEnabled(false);
    
    if (m_Controls->labelFiberMinLength->isEnabled())
      m_Controls->labelFiberMinLength->setEnabled(false);
    
    if (m_Controls->boxFiberMaxLength->isEnabled())
      m_Controls->boxFiberMaxLength->setEnabled(false);
    
    if (m_Controls->labelFiberMaxLength->isEnabled())
      m_Controls->labelFiberMaxLength->setEnabled(false);
    
    //enable radiobuttons
    if (!m_Controls->labelFibersTotal->isEnabled())
      m_Controls->labelFibersTotal->setEnabled(true);
    
    if (!m_Controls->boxFiberNumbers->isEnabled())
      m_Controls->boxFiberNumbers->setEnabled(true);
    
    if (!m_Controls->labelDistrRadius->isEnabled())
      m_Controls->labelDistrRadius->setEnabled(true);
    
    if (!m_Controls->boxDistributionRadius->isEnabled())
      m_Controls->boxDistributionRadius->setEnabled(true);
    
    
  } else {
    // disable radiobuttons    
    if (m_Controls->labelDistrRadius->isEnabled())
      m_Controls->labelDistrRadius->setEnabled(false);
    
    if (m_Controls->boxDistributionRadius->isEnabled())
      m_Controls->boxDistributionRadius->setEnabled(false);
    
    
    //enable radiobuttons
    if (!m_Controls->labelFibersTotal->isEnabled())
      m_Controls->labelFibersTotal->setEnabled(true);
    
    if (!m_Controls->boxFiberNumbers->isEnabled())
      m_Controls->boxFiberNumbers->setEnabled(true);
    
    if (!m_Controls->boxFiberMinLength->isEnabled())
      m_Controls->boxFiberMinLength->setEnabled(true);
    
    if (!m_Controls->labelFiberMinLength->isEnabled())
      m_Controls->labelFiberMinLength->setEnabled(true);
    
    if (!m_Controls->boxFiberMaxLength->isEnabled())
      m_Controls->boxFiberMaxLength->setEnabled(true);
    
    if (!m_Controls->labelFiberMaxLength->isEnabled())
      m_Controls->labelFiberMaxLength->setEnabled(true);
    
  }   
  
}

void QmitkFiberBundleDeveloperView::DoGenerateFibers()
{
  
  // GET SELECTED FIBER DIRECTION
  QString fibDirection; //stores the object_name of selected radiobutton 
  QVector<QRadioButton*>::const_iterator i;
  for (i = m_DirectionRadios.begin(); i != m_DirectionRadios.end(); ++i) 
  {
    QRadioButton* rdbtn = *i;
    if (rdbtn->isChecked())
      fibDirection = rdbtn->objectName();
  }
  
  //  vtkPolyData* output; // FiberPD stores the generated PolyData... going to be generated in thread
  if ( fibDirection == FIB_RADIOBUTTON_DIRECTION_RANDOM ) {
    //    build polydata with random lines and fibers
    //    output = 
    GenerateVtkFibersRandom();
    
  } else if ( fibDirection == FIB_RADIOBUTTON_DIRECTION_X ) {
    //    build polydata with XDirection fibers
    //output = GenerateVtkFibersDirectionX();
    
  } else if ( fibDirection == FIB_RADIOBUTTON_DIRECTION_Y ) {
    //    build polydata with YDirection fibers
    // output = GenerateVtkFibersDirectionY();
    
  } else if ( fibDirection == FIB_RADIOBUTTON_DIRECTION_Z ) {
    //    build polydata with ZDirection fibers
    //  output = GenerateVtkFibersDirectionZ();
    
  }
  
  
} 

void QmitkFiberBundleDeveloperView::PutFibersToDataStorage( vtkPolyData* threadOutput)
{
  
  MITK_INFO << "lines: " << threadOutput->GetNumberOfLines() << "pnts: " << threadOutput->GetNumberOfPoints();
  //qthread mutex lock
  mitk::FiberBundleX::Pointer FB = mitk::FiberBundleX::New();
  FB->SetFibers(threadOutput);
  FB->SetGeometry(this->GenerateStandardGeometryForMITK());
  
  mitk::DataNode::Pointer FBNode;
  FBNode = mitk::DataNode::New();
  FBNode->SetName("FiberBundleX");
  FBNode->SetData(FB);
  FBNode->SetVisibility(true);
  
  GetDataStorage()->Add(FBNode);
  //output->Delete();
  
  const mitk::PlaneGeometry * tsgeo = m_MultiWidget->GetTimeNavigationController()->GetCurrentPlaneGeometry();	
  if (tsgeo == NULL) {
    /* GetDataStorage()->Modified etc. have no effect, therefore proceed as followed below */
    // get all nodes that have not set "includeInBoundingBox" to false
    mitk::NodePredicateNot::Pointer pred = mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("includeInBoundingBox"
                                                                                                        , mitk::BoolProperty::New(false)));
    mitk::DataStorage::SetOfObjects::ConstPointer rs = GetDataStorage()->GetSubset(pred);
    // calculate bounding geometry of these nodes
    mitk::TimeSlicedGeometry::Pointer bounds = GetDataStorage()->ComputeBoundingGeometry3D(rs);
    // initialize the views to the bounding geometry
    mitk::RenderingManager::GetInstance()->InitializeViews(bounds);
  } else {
    
    GetDataStorage()->Modified();
    m_MultiWidget->RequestUpdate(); //necessary??
  }
  
  //qthread mutex unlock
}

/*
 * Generate polydata of random fibers
 */
void QmitkFiberBundleDeveloperView::GenerateVtkFibersRandom()
{
  
  /* ===== TIMER CONFIGURATIONS for visual effect ======
   * start and stop is called in Thread */
  QTimer *localTimer = new QTimer; // timer must be initialized here, otherwise timer is not fancy enough
  localTimer->setInterval( 10 );
  connect( localTimer, SIGNAL(timeout()), this, SLOT(UpdateGenerateRandomFibersTimer()) );
  
  struct Package4WorkingThread ItemPackageForRandomGenerator;
  ItemPackageForRandomGenerator.st_FBX = m_FiberBundleX;
  ItemPackageForRandomGenerator.st_Controls = m_Controls;
  ItemPackageForRandomGenerator.st_FancyGUITimer1 = localTimer;
  ItemPackageForRandomGenerator.st_host = this;
  ItemPackageForRandomGenerator.st_pntr_to_Method_PutFibersToDataStorage = &QmitkFiberBundleDeveloperView::PutFibersToDataStorage;
  
  if (m_threadInProgress)
    return; //maybe popup window saying, working thread still in progress...pls wait
  
  m_GeneratorFibersRandom = new QmitkFiberGenerateRandomWorker(m_hostThread, ItemPackageForRandomGenerator);
  m_GeneratorFibersRandom->moveToThread(m_hostThread);
  
  connect(m_hostThread, SIGNAL(started()), this, SLOT( BeforeThread_GenerateFibersRandom()) );
  connect(m_hostThread, SIGNAL(started()), m_GeneratorFibersRandom, SLOT(run()) );
  connect(m_hostThread, SIGNAL(finished()), this, SLOT(AfterThread_GenerateFibersRandom()) );
  connect(m_hostThread, SIGNAL(terminated()), this, SLOT(AfterThread_GenerateFibersRandom()) );
  
  m_hostThread->start(QThread::LowestPriority);
  
}

void QmitkFiberBundleDeveloperView::UpdateGenerateRandomFibersTimer()
{
  //MAKE SURE by yourself THAT NOTHING ELSE THAN A NUMBER IS SET IN THAT LABEL
  QString crntValue = m_Controls->infoTimerGenerateFiberBundle->text();
  int tmpVal = crntValue.toInt();
  m_Controls->infoTimerGenerateFiberBundle->setText(QString::number(++tmpVal));  
  m_Controls->infoTimerGenerateFiberBundle->update();
  
}

void QmitkFiberBundleDeveloperView::BeforeThread_GenerateFibersRandom()
{
  m_threadInProgress = true;
  m_fiberThreadMonitorWorker->threadForFiberProcessingStarted();
}

void QmitkFiberBundleDeveloperView::AfterThread_GenerateFibersRandom()
{
  m_threadInProgress = false;
  //m_fiberThreadMonitorWorker->sayGoodbye();//dummy implementationof purpose
  
  disconnect(m_hostThread, 0, 0, 0);
  m_hostThread->disconnect();
}

vtkSmartPointer<vtkPolyData> QmitkFiberBundleDeveloperView::GenerateVtkFibersDirectionX()
{
  int numOfFibers = m_Controls->boxFiberNumbers->value();  
  vtkSmartPointer<vtkCellArray> linesCell = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  
  //insert Origin point, this point has index 0 in point array
  double originX = 0.0;
  double originY = 0.0;
  double originZ = 0.0;
  
  //after each iteration the origin of the new fiber increases
  //here you set which direction is affected.
  double increaseX = 0.0;
  double increaseY = 1.0;
  double increaseZ = 0.0;
  
  
  //walk along X axis
  //length of fibers increases in each iteration
  for (int i=0; i<numOfFibers; ++i) {
    
    vtkSmartPointer<vtkPolyLine> newFiber = vtkSmartPointer<vtkPolyLine>::New();
    newFiber->GetPointIds()->SetNumberOfIds(i+2);
    
    //create starting point and add it to pointset
    points->InsertNextPoint(originX + (double)i * increaseX , originY + (double)i * increaseY, originZ + (double)i * increaseZ);
    
    //add starting point to fiber
    newFiber->GetPointIds()->SetId(0,points->GetNumberOfPoints()-1);
    
    //insert remaining points for fiber
    for (int pj=0; pj<=i ; ++pj) 
    { //generate next point on X axis
      points->InsertNextPoint( originX + (double)pj+1 , originY + (double)i * increaseY, originZ + (double)i * increaseZ );
      newFiber->GetPointIds()->SetId(pj+1,points->GetNumberOfPoints()-1);
    }
    
    linesCell->InsertNextCell(newFiber);
  }
  
  vtkSmartPointer<vtkPolyData> PDX = vtkSmartPointer<vtkPolyData>::New(); 
  PDX->SetPoints(points);
  PDX->SetLines(linesCell);
  return PDX;
}

vtkSmartPointer<vtkPolyData> QmitkFiberBundleDeveloperView::GenerateVtkFibersDirectionY()
{
  vtkSmartPointer<vtkPolyData> PDY = vtkSmartPointer<vtkPolyData>::New();
  //todo
  
  
  
  return PDY;
}


vtkSmartPointer<vtkPolyData> QmitkFiberBundleDeveloperView::GenerateVtkFibersDirectionZ()
{
  vtkSmartPointer<vtkPolyData> PDZ = vtkSmartPointer<vtkPolyData>::New();
  //todo
  
  
  
  return PDZ;
}







/* === OutSourcedMethod: THIS METHOD GENERATES ESSENTIAL GEOMETRY PARAMETERS FOR THE MITK FRAMEWORK ===
 * WITHOUT, the rendering mechanism will ignore objects without valid Geometry 
 * for each object, MITK requires: ORIGIN, SPACING, TRANSFORM MATRIX, BOUNDING-BOX */
mitk::Geometry3D::Pointer QmitkFiberBundleDeveloperView::GenerateStandardGeometryForMITK()
{
  mitk::Geometry3D::Pointer geometry = mitk::Geometry3D::New();
  
  // generate origin
  mitk::Point3D origin;
  origin[0] = 0;
  origin[1] = 0;
  origin[2] = 0;
  geometry->SetOrigin(origin);
  
  // generate spacing
  float spacing[3] = {1,1,1};
  geometry->SetSpacing(spacing);
  
  
  // generate identity transform-matrix
  vtkMatrix4x4* m = vtkMatrix4x4::New();
  geometry->SetIndexToWorldTransformByVtkMatrix(m);
  
  // generate boundingbox
  // for an usable bounding-box use gui parameters to estimate the boundingbox
  float bounds[] = {500, 500, 500, -500, -500, -500};
  
  // GET SELECTED FIBER DIRECTION
  QString fibDirection; //stores the object_name of selected radiobutton 
  QVector<QRadioButton*>::const_iterator i;
  for (i = m_DirectionRadios.begin(); i != m_DirectionRadios.end(); ++i) 
  {
    QRadioButton* rdbtn = *i;
    if (rdbtn->isChecked())
      fibDirection = rdbtn->objectName();
  }
  
  if ( fibDirection == FIB_RADIOBUTTON_DIRECTION_RANDOM ) {
    // use information about distribution parameter to calculate bounding box
    int distrRadius = m_Controls->boxDistributionRadius->value();
    bounds[0] = distrRadius; 
    bounds[1] = distrRadius;
    bounds[2] = distrRadius;
    bounds[3] = -distrRadius;
    bounds[4] = -distrRadius;
    bounds[5] = -distrRadius;
    
  } else {
    // so far only X,Y,Z directions are available
    MITK_INFO << "_______GEOMETRY ISSUE_____\n***BoundingBox for X, Y, Z fiber directions are not optimized yet!***";
    
    int maxFibLength = m_Controls->boxFiberMaxLength->value();
    bounds[0] = maxFibLength;
    bounds[1] = maxFibLength;
    bounds[2] = maxFibLength;
    bounds[3] = -maxFibLength;
    bounds[4] = -maxFibLength;
    bounds[5] = -maxFibLength;
  }
  
  geometry->SetFloatBounds(bounds);
  geometry->SetImageGeometry(true); //??
  
  return geometry;
  
  
}

void QmitkFiberBundleDeveloperView::UpdateFiberIDTimer()
{
  //MAKE SURE by yourself THAT NOTHING ELSE THAN A NUMBER IS SET IN THAT LABEL
  QString crntValue = m_Controls->infoTimerGenerateFiberIds->text();
  int tmpVal = crntValue.toInt();
  m_Controls->infoTimerGenerateFiberIds->setText(QString::number(++tmpVal));  
  m_Controls->infoTimerGenerateFiberIds->update();
  
}

/* Initialie ID dataset in FiberBundleX */
void QmitkFiberBundleDeveloperView::DoGenerateFiberIDs()
{
  
  /* ===== TIMER CONFIGURATIONS for visual effect ======
   * start and stop is called in Thread */
  QTimer *localTimer = new QTimer; // timer must be initialized here, otherwise timer is not fancy enough
  localTimer->setInterval( 10 );
  connect( localTimer, SIGNAL(timeout()), this, SLOT(UpdateFiberIDTimer()) );
  
  
  // pack items which are needed by thread processing
  struct Package4WorkingThread FiberIdPackage;
  FiberIdPackage.st_FBX = m_FiberBundleX;
  FiberIdPackage.st_FancyGUITimer1 = localTimer;
  FiberIdPackage.st_Controls = m_Controls;
  
  
  if (m_threadInProgress)
    return; //maybe popup window saying, working thread still in progress...pls wait
  
  // THREAD CONFIGURATION
  m_FiberIDGenerator = new QmitkFiberIDWorker(m_hostThread, FiberIdPackage);
  m_FiberIDGenerator->moveToThread(m_hostThread);
  connect(m_hostThread, SIGNAL(started()), this, SLOT( BeforeThread_IdGenerate()) );
  connect(m_hostThread, SIGNAL(started()), m_FiberIDGenerator, SLOT(run()));
  connect(m_hostThread, SIGNAL(finished()), this, SLOT(AfterThread_IdGenerate()));
  connect(m_hostThread, SIGNAL(terminated()), this, SLOT(AfterThread_IdGenerate()));
  m_hostThread->start(QThread::LowestPriority);
  
  
  
  // m_Controls->infoTimerGenerateFiberIds->setText(QString::number(clock.GetTotal()));
  
}

void QmitkFiberBundleDeveloperView::BeforeThread_IdGenerate()
{
  m_threadInProgress = true;
  
}

void QmitkFiberBundleDeveloperView::AfterThread_IdGenerate()
{
  
  
  m_threadInProgress = false;
  disconnect(m_hostThread, 0, 0, 0);
  m_hostThread->disconnect();
  
}


void  QmitkFiberBundleDeveloperView::ResetFiberInfoWidget()
{
  if (m_Controls->infoAnalyseNumOfFibers->isEnabled()) {
    m_Controls->infoAnalyseNumOfFibers->setText("-");
    m_Controls->infoAnalyseNumOfPoints->setText("-");
    m_Controls->infoAnalyseNumOfFibers->setEnabled(false);
  }
}

void  QmitkFiberBundleDeveloperView::FeedFiberInfoWidget()
{
  if (!m_Controls->infoAnalyseNumOfFibers->isEnabled())
    m_Controls->infoAnalyseNumOfFibers->setEnabled(true);
  
  QString numOfFibers;
  numOfFibers.setNum( m_FiberBundleX->GetFibers()->GetNumberOfLines() );
  QString numOfPoints;
  numOfPoints.setNum( m_FiberBundleX->GetFibers()->GetNumberOfPoints() );
  
  m_Controls->infoAnalyseNumOfFibers->setText( numOfFibers );
  m_Controls->infoAnalyseNumOfPoints->setText( numOfPoints );
}

void QmitkFiberBundleDeveloperView::SelectionChangedToolBox(int idx)
{
  MITK_INFO << "printtoolbox: " << idx;
  if (m_Controls->page_FiberInfo->isVisible() && m_FiberBundleX != NULL)
  {
    FeedFiberInfoWidget();
    
  } else {
    //if infolables are disabled: return
    //else set info back to - and set label and info to disabled
    
    if (!m_Controls->page_FiberInfo->isVisible()) {
      return;
    } else {
      ResetFiberInfoWidget();
    }
  }
  
}

void QmitkFiberBundleDeveloperView::FBXDependendGUIElementsConfigurator(bool isVisible)
{
  // ==== FIBER PROCESSING ELEMENTS ======
  m_Controls->buttonGenerateFiberIds->setEnabled(isVisible);
  
  
}


void QmitkFiberBundleDeveloperView::DoMonitorFiberThreads(int checkStatus)
{
  //check if in datanode exists already a node of type mitkFiberBundleXThreadMonitor
  //if not then put node to datastorage
  
  //if checkStatus is 1 then start qtimer using fading in starting text in datanode
  //if checkStatus is 0 then fade out dataNode using qtimer
  
  if (checkStatus)
  {
    
    // Generate Node hosting thread information
    mitk::FiberBundleXThreadMonitor::Pointer FBXThreadMonitor = mitk::FiberBundleXThreadMonitor::New();
    FBXThreadMonitor->SetGeometry(this->GenerateStandardGeometryForMITK());
    QString str = "Aloha";
    FBXThreadMonitor->setTextL1(str);
    
    m_MonitorNode = mitk::DataNode::New();
    m_MonitorNode->SetName("FBX_threadMonitor");
    m_MonitorNode->SetData(FBXThreadMonitor);
    m_MonitorNode->SetVisibility(true);
    m_MonitorNode->SetOpacity(1.0);
    
    GetDataStorage()->Add(m_MonitorNode);
    
    //following code is needed for rendering text in mitk! without geometry nothing is rendered
    const mitk::PlaneGeometry * tsgeo = m_MultiWidget->GetTimeNavigationController()->GetCurrentPlaneGeometry();	
    if (tsgeo == NULL) {
      /* GetDataStorage()->Modified etc. have no effect, therefore proceed as followed below */
      // get all nodes that have not set "includeInBoundingBox" to false
      mitk::NodePredicateNot::Pointer pred = mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New( "includeInBoundingBox"
                                                                                                          , mitk::BoolProperty::New(false)));
      mitk::DataStorage::SetOfObjects::ConstPointer rs = GetDataStorage()->GetSubset(pred);
      // calculate bounding geometry of these nodes
      mitk::TimeSlicedGeometry::Pointer bounds = GetDataStorage()->ComputeBoundingGeometry3D(rs);
      // initialize the views to the bounding geometry
      mitk::RenderingManager::GetInstance()->InitializeViews(bounds);
    } else {
      
      GetDataStorage()->Modified();
      m_MultiWidget->RequestUpdate(); //necessary??
    }
    //__GEOMETRY FOR THREADMONITOR GENERATED
    
    /* ====== initialize thread for managing fiberThread information ========= */
    m_monitorThread = new QThread;
    // the package needs datastorage, MonitorDatanode, standardmultiwidget,  
    struct Package4WorkingThread ItemPackageForThreadMonitor;
    ItemPackageForThreadMonitor.st_DataStorage = GetDataStorage();
    ItemPackageForThreadMonitor.st_ThreadMonitorDataNode = m_MonitorNode;
    ItemPackageForThreadMonitor.st_MultiWidget = m_MultiWidget;
    ItemPackageForThreadMonitor.st_FBX_Monitor = FBXThreadMonitor;
    
    m_fiberThreadMonitorWorker = new QmitkFiberThreadMonitorWorker(m_monitorThread, ItemPackageForThreadMonitor);
    
    m_fiberThreadMonitorWorker->moveToThread(m_monitorThread);
    connect ( m_monitorThread, SIGNAL( started() ), m_fiberThreadMonitorWorker, SLOT( run() ) );
    m_monitorThread->start(QThread::LowestPriority);
    m_fiberThreadMonitorWorker->initializeMonitor();//do some init animation ;-)

    
  } else {
    m_monitorThread->quit();
    //think about outsourcing following lines to quit / terminate slot of thread
    GetDataStorage()->Remove(m_MonitorNode);
    GetDataStorage()->Modified();
    m_MultiWidget->RequestUpdate(); //necessary??
  }
  
  
   
}



void QmitkFiberBundleDeveloperView::StdMultiWidgetAvailable (QmitkStdMultiWidget &stdMultiWidget)
{
  m_MultiWidget = &stdMultiWidget;
}


void QmitkFiberBundleDeveloperView::StdMultiWidgetNotAvailable()
{
  m_MultiWidget = NULL;
}

/* OnSelectionChanged is registered to SelectionService, therefore no need to
 implement SelectionService Listener explicitly */
void QmitkFiberBundleDeveloperView::OnSelectionChanged( std::vector<mitk::DataNode*> nodes )
{ 
  
  /* ==== reset everyhing related to FiberBundleX ======
   * - variable m_FiberBundleX
   * - visualization of analysed fiberbundle
   */
  m_FiberBundleX = NULL; //reset pointer, so that member does not point to depricated locations
  ResetFiberInfoWidget();
  FBXDependendGUIElementsConfigurator(false); //every gui element which needs a FBX for processing is disabled
  
  //timer reset only when no thread is in progress
  if (!m_threadInProgress) {
    m_Controls->infoTimerGenerateFiberIds->setText("-"); //set GUI representation of timer to -
    m_Controls->infoTimerGenerateFiberBundle->setText( "-" );
  }
 //====================================================
  
  
  if (nodes.empty())
    return;
  
  
  for( std::vector<mitk::DataNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it )
  {
    mitk::DataNode::Pointer node = *it;
    
    /* CHECKPOINT: FIBERBUNDLE*/
    if( node.IsNotNull() && dynamic_cast<mitk::FiberBundleX*>(node->GetData()) )
    {
      m_FiberBundleX = dynamic_cast<mitk::FiberBundleX*>(node->GetData());
      if (m_FiberBundleX == NULL)
        MITK_INFO << "========ATTENTION=========\n unable to load selected FiberBundleX to FiberBundleDeveloper-plugin \n";
      
      // ==== FIBERBUNDLE_INFO ELEMENTS ====
      if ( m_Controls->page_FiberInfo->isVisible() )
        FeedFiberInfoWidget(); 
      
      
      // enable FiberBundleX related Gui Elements, such as buttons etc.
      FBXDependendGUIElementsConfigurator(true);
      
    }
    
    
    
    
  }
}

void QmitkFiberBundleDeveloperView::Activated()
{
  
  MITK_INFO << "FB DevelopersV ACTIVATED()";
  
  
}



