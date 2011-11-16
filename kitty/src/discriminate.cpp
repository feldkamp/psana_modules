//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class discriminate...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/discriminate.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <iomanip>
using std::setw;

#include <vector>
using std::vector;

#include <string>
using std::string;

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "PSEvt/EventId.h"

#include "kitty/constants.h"

#include "kitty/util.h"


//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

// This declares this class as psana module
using namespace kitty;
PSANA_MODULE_FACTORY(discriminate)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
discriminate::discriminate (const std::string& name)
	: Module(name)
	, p_useCorrectedData(0)
	, p_lowerThreshold(0)
	, p_upperThreshold(0)
	, p_discriminateAlogrithm(0)
	, p_outputPrefix("")
	, p_pixelVectorOutput(0)
	, p_maxHits(0)
	, p_skipcount(0)
	, p_hitcount(0)
	, p_count(0)
	, p_stopflag(false)
	, p_useShift(0)	
	, p_shiftX(0)
	, p_shiftY(0)
	, p_detOffset(0)
	, p_sum_sp()
	, p_hitInt()
	, m_dataSourceString("")
	, m_calibSourceString("")
	, m_calibDir("")
	, m_typeGroupName("")
	, m_runNumber(0)
	, m_tiltIsApplied(true)
	, m_cspad_calibpar()
	, m_pix_coords_2x1()
	, m_pix_coords_quad()
	, m_pix_coords_cspad()
{
	// get the values from configuration or use defaults
	m_dataSourceString			= configStr("dataSource",			"CxiDs1.0:Cspad.0");
	m_calibSourceString			= configStr("calibSource",			"CxiDs1.0:Cspad.0");
	m_calibDir      			= configStr("calibDir",				"/reg/d/psdm/CXI/cxi35711/calib");
	m_typeGroupName 			= configStr("typeGroupName",		"CsPad::CalibV1");
	m_tiltIsApplied 			= config   ("tiltIsApplied",		true);			//tilt angle correction
	m_runNumber					= config   ("calibRunNumber",     	0);
	
	p_lowerThreshold 			= config("lowerThreshold", 			-10000000);
	p_upperThreshold 			= config("upperThreshold", 			10000000);
	p_discriminateAlogrithm 	= config("discriminateAlgorithm", 	0);
	p_maxHits					= config("maxHits",					10000000);
	
	p_outputPrefix				= configStr("outputPrefix", 		"out");
	p_pixelVectorOutput			= config("pixelVectorOutput", 		0);
	
	p_useCorrectedData			= config("useCorrectedData",        1);
	p_useShift					= config("useShift",				1);
	p_shiftX					= config("shiftX",					-867.355);
	p_shiftY					= config("shiftY",					-862.758);
	p_detOffset 				= config("detOffset",				500.0 + 63.0);	// see explanation in header
	
	p_sum_sp = shared_ptr<array1D>( new array1D(nMaxTotalPx) );
}

//--------------
// Destructor --
//--------------
discriminate::~discriminate ()
{

	delete m_cspad_calibpar; 
	delete m_pix_coords_2x1;
	delete m_pix_coords_quad;
	delete m_pix_coords_cspad;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
discriminate::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "discriminate::beginJob()" );
	MsgLog(name(), info, "maxHits = " << p_maxHits );
	MsgLog(name(), info, "lowerThreshold = " << p_lowerThreshold );
	MsgLog(name(), info, "upperThreshold = " << p_upperThreshold );
	MsgLog(name(), info, "discriminateAlgorithm = " << p_discriminateAlogrithm );
	MsgLog(name(), info, "useShift = " << p_useShift );
	MsgLog(name(), info, "shiftX = " << p_shiftX );
	MsgLog(name(), info, "shiftY = " << p_shiftY );
	MsgLog(name(), info, "detOffset = " << p_detOffset );
	
	MsgLog(name(), debug, "------CSPAD info------" );
	MsgLog(name(), debug, "nRowsPerASIC = " << nRowsPerASIC );
	MsgLog(name(), debug, "nColsPerASIC = " << nColsPerASIC ); 
	MsgLog(name(), debug, "nRowsPer2x1 = " << nRowsPer2x1 );
	MsgLog(name(), debug, "nColsPer2x1 = " << nColsPer2x1 ); 
	MsgLog(name(), debug, "nMax2x1sPerQuad =  " << nMax2x1sPerQuad ); 
	MsgLog(name(), debug, "nMaxQuads = " << nMaxQuads ); 
	MsgLog(name(), debug, "nPxPer2x1 = " << nPxPer2x1 ); 
	MsgLog(name(), debug, "nMaxPxPerQuad = " << nMaxPxPerQuad ); 
	MsgLog(name(), debug, "nMaxTotalPx = " << nMaxTotalPx ); 
}




/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
discriminate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::beginRun()" );
	
	//put output string into event to make it accessible for all modules
	shared_ptr<std::string> outputPrefix_sp( new std::string(p_outputPrefix) );
	evt.put(outputPrefix_sp, IDSTRING_OUTPUT_PREFIX);

	MsgLog(name(), debug, "------list of stored PVs------" );
	const vector<string>& pvNames = env.epicsStore().pvNames();
	for (unsigned int i = 0; i < pvNames.size(); i++){
		MsgLog(name(), debug, pvNames.at(i) );
	}
	
	MsgLog(name(), info, "------list of read out PVs------" );
	//compile a list of general PVs that need to be read out for this job (and don't change for each event)
	vector<string> runPVs;						// vector for list of PVs
	vector<string> runDesc;						// vector for PV description
	
	runPVs.push_back("CXI:DS1:MMS:06");									//# 0
	runDesc.push_back("detector position");
	unsigned int detZ_no = 0;
	
	runPVs.push_back("BEND:DMP1:400:BDES");								//# 1
	runDesc.push_back("electron beam energy");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO289");							//# 2
	runDesc.push_back("Vernier energy");
	
	runPVs.push_back("BPMS:DMP1:199:TMIT1H");							//# 3
	runDesc.push_back("particle N_electrons");

	runPVs.push_back("EVNT:SYS0:1:LCLSBEAMRATE");						//# 4
	runDesc.push_back("LCLS repetition rate [Hz]");
		
	runPVs.push_back("SIOC:SYS0:ML00:AO195");							//# 5
	runDesc.push_back("peak current after second bunch compressor");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO820");							//# 6
	runDesc.push_back("pulse length");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO569");							//# 7
	runDesc.push_back("ebeam energy loss converted to photon mJ");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO580");							//# 8
	runDesc.push_back("calculated number of photons");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO541");							//# 9
	runDesc.push_back("photon beam energy [eV]");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO627");							//# 10
	runDesc.push_back("photon beam energy");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO192");							//# 11
	runDesc.push_back("wavelength [nm]");
	unsigned int lambda_no = 11;
	
	
	vector<double> runVals;						// vector for corresponding values
	runVals.assign(runPVs.size(), 0.);			// set all values of PVs to zero initially
	vector<bool> runPVvalid;					// keep track of the validity of this PV
	runPVvalid.assign(runPVs.size(), false);
	
	for (unsigned int i = 0; i < runPVs.size(); i++){
		try{
			runVals.at(i) = env.epicsStore().value(runPVs.at(i));
			MsgLog(name(), info, setw(3) << "#" << i << " " 
				<< setw(26) << runPVs.at(i) << " = " 
				<< setw(12) << runVals.at(i) 
				<< " (" << runDesc.at(i) << ")" );
			runPVvalid.at(i) = true;
		}catch(...){
			MsgLog(name(), warning, "PV " << runPVs.at(i) << " (" << runDesc.at(i) << ") doesn't exist." );
			runPVvalid.at(i) = false;
		}
	}
	
	MsgLog(name(), info, "------quantities derived from selected PVs------" );
	double detZ = 0; 
	if (runPVvalid.at(detZ_no) ){
		detZ = p_detOffset + runVals.at(detZ_no);
	}
	double lambda = 0;
	if (runPVvalid.at(lambda_no) ){
		lambda = runVals.at(lambda_no);
	}
	double two_k = 2*2*M_PI/lambda;								//in units of inv. nanometers
	MsgLog(name(), info, "detZ = " << detZ << " mm");				
	MsgLog(name(), info, "2*k = " << two_k << "nm^-1" );
	MsgLog(name(), info, " ");
	
	std::list<EventKey> keys = evt.keys();
	MsgLog(name(), info, "------" << keys.size() << " keys in event------");
	for ( std::list<EventKey>::iterator it = keys.begin(); it != keys.end(); it++ ){
		MsgLog(name(), debug, "key: '" << it->key() << "'");
	}
	

	//get run rumber for this run
	shared_ptr<EventId> eventId = evt.get();
    if (eventId.get()) {
		m_runNumber = eventId->run();
		MsgLog(name(), trace, name() << ": Using run number " << m_runNumber);
    } else {
		MsgLog(name(), warning, name() << ": Cannot determine run number, will use 0.");
		m_runNumber = 0;
    }
	
	//look up calibration data (CSPAD geometry) for this run
	try{
		MsgLog(name(), info, "reading calibration using "
								<< "\n\t\t\t calib dir  = '" << m_calibDir << "'"
								<< "\n\t\t\t type group = '" << m_typeGroupName << "'"
								<< "\n\t\t\t source     = '" << m_calibSourceString << "'"
								<< "\n\t\t\t run number = '" << m_runNumber << "'"
								);
		m_cspad_calibpar   = new PSCalib::CSPadCalibPars(m_calibDir, m_typeGroupName, m_calibSourceString, m_runNumber);
		m_pix_coords_2x1   = new CSPadPixCoords::PixCoords2x1();
		m_pix_coords_quad  = new CSPadPixCoords::PixCoordsQuad( m_pix_coords_2x1,  m_cspad_calibpar, m_tiltIsApplied );
		m_pix_coords_cspad = new CSPadPixCoords::PixCoordsCSPad( m_pix_coords_quad, m_cspad_calibpar, m_tiltIsApplied );
	
		//check psana's debug level
		//....so we can get rid of the verbosity when we actually run a lot of data
		MsgLogger::MsgLogLevel lvl(MsgLogger::MsgLogLevel::debug);
		if ( MsgLogger::MsgLogger().logging(lvl) ) {
			cout << "------------calibration data parser---------------" << endl;
			m_cspad_calibpar->printCalibPars();
			cout << "------------pix coords 2x1-----------" << endl;
			m_pix_coords_2x1->print_member_data();
		}
	}catch(...){
		MsgLog(name(), error, "Could not retrieve calibration data");
	}
	
	//---read calibration information arrays---
	//---possibly take some of this out if not needed and memory is needed
	//
	// in microns
	shared_ptr<array1D> pixX_um_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrX_um(), nMaxTotalPx) );
	shared_ptr<array1D> pixY_um_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrY_um(), nMaxTotalPx) );
	
	// in pixel index (integer pixel position only)
	shared_ptr<array1D> pixX_int_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrX_int(), nMaxTotalPx) );
	shared_ptr<array1D> pixY_int_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrY_int(), nMaxTotalPx) );

	// in pix (pixel index, including fraction)
	shared_ptr<array1D> pixX_pix_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrX_pix(), nMaxTotalPx) );
	shared_ptr<array1D> pixY_pix_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrY_pix(), nMaxTotalPx) );

	
	
	MsgLog(name(), debug, "------read pixel arrays from calib data------");	
	MsgLog(name(), debug, "PixCoorArrX_um  min: " << pixX_um_sp->calcMin()  << ", max: " << pixX_um_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrY_um  min: " << pixY_um_sp->calcMin()  << ", max: " << pixY_um_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrX_int min: " << pixX_int_sp->calcMin() << ", max: " << pixX_int_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrY_int min: " << pixY_int_sp->calcMin() << ", max: " << pixY_int_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrX_pix min: " << pixX_pix_sp->calcMin() << ", max: " << pixX_pix_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrY_pix min: " << pixY_pix_sp->calcMin() << ", max: " << pixY_pix_sp->calcMax() );
	
	//shift pixel arrays to be centered around incoming beam at (0, 0)
	//this only works exactly, when the beam was exactly centered in the CSPAD
	//otherwise, use manual correction of shiftX & shiftY below
	double shift_X_um = (pixX_um_sp->calcMin()+pixX_um_sp->calcMax())/2;
	double shift_Y_um = (pixY_um_sp->calcMin()+pixY_um_sp->calcMax())/2;
	double shift_X_int = (pixX_int_sp->calcMin()+pixX_int_sp->calcMax())/2;
	double shift_Y_int = (pixY_int_sp->calcMin()+pixY_int_sp->calcMax())/2;
	double shift_X_pix = (pixX_pix_sp->calcMin()+pixX_pix_sp->calcMax())/2;
	double shift_Y_pix = (pixY_pix_sp->calcMin()+pixY_pix_sp->calcMax())/2;
	
	if (p_useShift){
		//pixel size values estimated from the read out arrays above, seems about right
		const double pixelSizeXY_um = m_cspad_calibpar->getColSize_um();		// == getRowSize_um(), square pixels (luckily)
		shift_X_um = p_shiftX * pixelSizeXY_um;
		shift_Y_um = p_shiftX * pixelSizeXY_um;
		shift_X_int = p_shiftX;
		shift_Y_int = p_shiftY;
		shift_X_pix = p_shiftX;
		shift_Y_pix = p_shiftY;
	}
	
	pixX_um_sp->subtractValue( shift_X_um );
	pixY_um_sp->subtractValue( shift_Y_um );
	pixX_int_sp->subtractValue( shift_X_int );
	pixY_int_sp->subtractValue( shift_Y_int );
	pixX_pix_sp->subtractValue( shift_X_pix );
	pixY_pix_sp->subtractValue( shift_Y_pix );
	
	// create q-value vectors, (inverse nanometers)
	shared_ptr<array1D> pixX_q_sp ( new array1D(nMaxTotalPx) );
	shared_ptr<array1D> pixY_q_sp ( new array1D(nMaxTotalPx) );
	shared_ptr<array1D> pixTwoTheta_sp ( new array1D(nMaxTotalPx) );
	shared_ptr<array1D> pixPhi_sp ( new array1D(nMaxTotalPx) );
			
	for (unsigned int i = 0; i < pixX_q_sp->size(); i++){
		double x_um = pixX_um_sp->get(i);
		double y_um = pixY_um_sp->get(i);
		double r_um = sqrt( x_um*x_um + y_um*y_um );
		double twoTheta = atan( r_um/1000.0/detZ );
		double phi = atan( y_um/x_um );
		pixTwoTheta_sp->set(i, twoTheta);
		pixPhi_sp->set(i, phi);
		
		pixX_q_sp->set(i, two_k * sin(twoTheta/2) * cos(phi) );
		pixY_q_sp->set(i, two_k * sin(twoTheta/2) * sin(phi) );
	}
	
	MsgLog(name(), info, "------shifted pixels arrays------");	
	MsgLog(name(), info, "PixCoorArrX_um  min: " << pixX_um_sp->calcMin()  << ", max: " << pixX_um_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_um  min: " << pixY_um_sp->calcMin()  << ", max: " << pixY_um_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_int min: " << pixX_int_sp->calcMin() << ", max: " << pixX_int_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_int min: " << pixY_int_sp->calcMin() << ", max: " << pixY_int_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_pix min: " << pixX_pix_sp->calcMin() << ", max: " << pixX_pix_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_pix min: " << pixY_pix_sp->calcMin() << ", max: " << pixY_pix_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_q   min: " << pixX_q_sp->calcMin()   << ", max: " << pixX_q_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_q   min: " << pixY_q_sp->calcMin()   << ", max: " << pixY_q_sp->calcMax() );
	MsgLog(name(), info, "PixTwoTheta     min: " << pixTwoTheta_sp->calcMin()   << ", max: " << pixTwoTheta_sp->calcMax() );
	MsgLog(name(), info, "PixPhi          min: " << pixPhi_sp->calcMin()   << ", max: " << pixPhi_sp->calcMax() );
	
	//add the calibration arrays to the event for other modules to use
	evt.put( pixX_um_sp,  IDSTRING_PX_X_um);
	evt.put( pixY_um_sp,  IDSTRING_PX_Y_um);
	evt.put( pixX_int_sp, IDSTRING_PX_X_int);
	evt.put( pixY_int_sp, IDSTRING_PX_Y_int);
	evt.put( pixX_pix_sp, IDSTRING_PX_X_pix);
	evt.put( pixY_pix_sp, IDSTRING_PX_Y_pix);
	evt.put( pixX_q_sp,   IDSTRING_PX_X_q);
	evt.put( pixY_q_sp,   IDSTRING_PX_Y_q);
	evt.put( pixTwoTheta_sp,   IDSTRING_PX_TWOTHETA);
	evt.put( pixPhi_sp,   IDSTRING_PX_PHI);	
	
	if (p_pixelVectorOutput){
		arraydataIO *io = new arraydataIO();
		string ext = ".h5";
		array2D *two = 0;

		createRawImageCSPAD( pixX_um_sp.get(), two );		
		io->writeToFile( p_outputPrefix+"_pixX_um"+ext, two );
		
		createRawImageCSPAD( pixY_um_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_um"+ext, two );
		
		createRawImageCSPAD( pixX_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_int"+ext, two );
		
		createRawImageCSPAD( pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_int"+ext, two );
		
		createRawImageCSPAD( pixX_pix_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_pix"+ext, two );
		
		createRawImageCSPAD( pixY_pix_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_pix"+ext, two );
		
		createRawImageCSPAD( pixX_q_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_q"+ext, two );
		
		createRawImageCSPAD( pixY_q_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_q"+ext, two );
		
		createRawImageCSPAD( pixTwoTheta_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixTwoTheta"+ext, two );
		
		createRawImageCSPAD( pixPhi_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixPhi"+ext, two );
		
		delete two;
		delete io;
	}
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
discriminate::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
discriminate::event(Event& evt, Env& env)
{
	if (p_stopflag){
		MsgLog(name(), info, "Stopping analysis job after maximum number of hits (" << p_maxHits << ") has been reached");
		stop();
		return;
	}
	
	shared_ptr<EventId> eventId = evt.get();
	MsgLog(name(), debug, "event ID: " << *eventId);

	std::list<EventKey> keys = evt.keys();
	MsgLog(name(), debug, "------" << keys.size() << " keys in event------");
	for ( std::list<EventKey>::iterator it = keys.begin(); it != keys.end(); it++ ){
		string thiskey = it->key();
		Pds::Src thissrc = it->src();
		string thissrcstr = "";
		MsgLog(name(), debug, "key: '" << thiskey << "', src: '" << thissrcstr << "'" );
	}
	
	//compile a list of PVs that need to be read out for each event 
	// (works, but commented out since it's not needed right now)
//	vector<string> eventPVs;						// vector for list of PVs
//	eventPVs.push_back("BEAM:LCLS:ELEC:Q");
//	
//	vector<double> eventVals;						// vector for corresponding values
//	eventVals.assign(eventPVs.size(), 0.);			// set all to zero
//	
//	for (unsigned int i = 0; i < eventPVs.size(); i++){
//		try{
//			eventVals.at(i) = env.epicsStore().value(eventPVs.at(i));
//			MsgLog(name(), info, eventPVs.at(i) << " = " << eventVals.at(i) );
//		}catch(...){
//			MsgLog(name(), warning, "PV " << eventPVs.at(i) << " doesn't exist." );
//		}
//	}

	ostringstream evtinfo;
	evtinfo <<  "evt# " << p_count << ", ";

	std::ostringstream osst;
//	osst << "t" << eventId->time();
	osst << std::setfill('0') << std::setw(10) << p_count;
	shared_ptr<std::string> eventname_sp( new std::string(osst.str()) );
	evt.put(eventname_sp, IDSTRING_CUSTOM_EVENTNAME);
	
	//if no calibrated data was found in the event, use the raw data from the xtc file
	//(probably the module cspad_mod.CsPadCalib wasn't executed before)
	string IDSTRING_CALIBRATED = "calibrated";
	string IDSTRING_NO_CALIB = "";
	string specifier = IDSTRING_CALIBRATED;
	
	//test, if calibrated data was found for this event, otherwise, use non-calibrated version
	if ( ((shared_ptr<Psana::CsPad::DataV2>) evt.get(m_dataSourceString, IDSTRING_CALIBRATED)) ){
		evtinfo << "pre-calibrated data ";
		specifier = IDSTRING_CALIBRATED;
	}else{
		specifier = IDSTRING_NO_CALIB;
		evtinfo << "non-calibrated data ";
	}
	
	//get CSPAD data from evt object (out of XTC stream)
	shared_ptr<Psana::CsPad::DataV2> data = evt.get(m_dataSourceString, specifier);
	
	//create shared_ptr to an array1D object, which can then be passed to the following modules
	shared_ptr<array1D> raw1D_sp ( new array1D(nMaxTotalPx) );
	array1D* raw1D = raw1D_sp.get();
	
	if (data.get()){
		int nQuads = data->quads_shape().at(0);
		for (int q = 0; q < nQuads; ++q) {
			const Psana::CsPad::ElementV2& el = data->quads(q);			
			const int16_t* quad_data = el.data();
			std::vector<int> quad_shape = el.data_shape();
			int actual2x1sPerQuad = quad_shape.at(0);
			int actualPxPerQuad = actual2x1sPerQuad * nPxPer2x1;
			
			//go through the whole data of the quad, copy to array1D
			for (int i = 0; i < actualPxPerQuad; i++){
				raw1D->set( q*nMaxPxPerQuad +i, (double)quad_data[i] );
			}
		}//for all quads
	}else{
		MsgLog(name(), error, evtinfo.str() 
			<< " --> could not get data from CSPAD " << m_dataSourceString << ". Skipping this event." );
		p_skipcount++;
		p_count++;
		skip();
		return;
	}
	
	// 'thresholdingAvg' can be different kinds of averages, depending on the disciminating algorithm
	// the threshold limits are compared to this value in order to accept or reject this shot
	double thresholdingAvg = 0;
	
	//select algorithm to calculate the thresholding average
	if (p_discriminateAlogrithm == 1){				// NOT COMPLETELY TESTED, YET
		int max_pos = 0;
		array1D *shot_hist = new array1D();
		array1D *shot_bins = new array1D();
		//calculate a histogram of the non-negative values
		raw1D->getHistogramInBoundaries(shot_hist, shot_bins, 500, 0, 500);
		shot_hist->calcMax( max_pos );
		thresholdingAvg = shot_bins->get(max_pos);
		delete shot_bins;
		delete shot_hist;
	}else{
		//if no other algorithm is specified, use a simple average across the whole detector
		thresholdingAvg = raw1D->calcAvg();
	}
	
	evtinfo << "avg:" << setw(8) << thresholdingAvg << ", threshold delta (" 
			<< setw(8) << thresholdingAvg-p_lowerThreshold << "/"
			<< setw(8) << thresholdingAvg-p_upperThreshold << ") ";
			
	if ( thresholdingAvg <= p_lowerThreshold || thresholdingAvg >= p_upperThreshold ){
		evtinfo << " xxxxx REJECT xxxxx";
		p_skipcount++;
		skip();		// all downstream modules will not be called
	}else{
		evtinfo << " --> ACCEPT (hit# " << p_hitcount << ") <--";
		p_hitcount++;
		
		// add the data to the event so that other modules can access it
		evt.put(raw1D_sp, IDSTRING_CSPAD_DATA);
		
		// add to running sum
		p_sum_sp->addArrayElementwise( raw1D_sp.get() );
		
		// put intensity in hit array
		p_hitInt.push_back(thresholdingAvg);
	}

	// output collected information...
	MsgLog(name(), info, evtinfo.str());
	
	// gracefully stop analysis job when the specified number of hits has been found
	// stop() halts the analysis right away, so it is invoked on the next hit AFTER the desired number
	if (p_hitcount >= p_maxHits){
		p_stopflag = true;
	}
	
	// increment event counter
	p_count++;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
discriminate::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
discriminate::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
discriminate::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::endJob()" );

	//create average out of raw sum
	p_sum_sp->divideByValue( p_hitcount );

	MsgLog(name(), info, "---------------------------------------------------");
	MsgLog(name(), info, "processed events: " << p_count 
		<< ", hits: " << p_hitcount 
		<< ", skipped: " << p_skipcount 
		<< ", hitrate: " << p_hitcount/(double)p_count * 100 << "%." );

	MsgLog(name(), debug, "---complete histogram of averaged data---\n" << p_sum_sp->getHistogramASCII(50) );
		
	arraydata *hits = new arraydata(p_hitInt);
	MsgLog(name(), info, "\n ------hit intensity histogram------\n" << hits->getHistogramASCII(30) );
	delete hits;
}



} // namespace kitty
