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
	, p_maxHits(0)
	, p_skipcount(0)
	, p_hitcount(0)
	, p_count(0)
	, p_useShift(0)	
	, p_shiftX(0)
	, p_shiftY(0)
	, p_detOffset(0)
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
	m_dataSourceString	= configStr("dataSource",			"CxiDs1.0:Cspad.0");
	m_calibSourceString	= configStr("calibSource",			"CxiDs1.0:Cspad.0");
	m_calibDir      	= configStr("calibDir",				"/reg/d/psdm/CXI/cxi35711/calib");
	m_typeGroupName 	= configStr("typeGroupName",		"CsPad::CalibV1");
	m_tiltIsApplied 	= config   ("tiltIsApplied",		true);								//tilt angle correction
	m_runNumber			= config   ("runNumber",     		0);
	
	p_lowerThreshold 	= config("lowerThreshold", 			0);
	p_upperThreshold 	= config("upperThreshold", 			10000000);
	p_maxHits			= config("maxHits",					10000000);
	
	p_useCorrectedData  = config("useCorrectedData",        1);
	p_useShift			= config("useShift",				1);
	p_shiftX			= config("shiftX",					-867.355);
	p_shiftY			= config("shiftY",					-862.758);
	p_detOffset 		= config("detOffset",				500.0 + 63.0);						// see explanation in header

	try{
		MsgLog(name, info, "reading calibration from "
								<< "\n   calib dir  = '" << m_calibDir << "'"
								<< "\n   type group = '" << m_typeGroupName << "'"
								<< "\n   source     = '" << m_calibSourceString << "'"
								<< "\n   run number = '" << m_runNumber << "'"
								);
		m_cspad_calibpar   = new PSCalib::CSPadCalibPars(m_calibDir, m_typeGroupName, m_calibSourceString, m_runNumber);
		m_pix_coords_2x1   = new CSPadPixCoords::PixCoords2x1();
		m_pix_coords_quad  = new CSPadPixCoords::PixCoordsQuad( m_pix_coords_2x1,  m_cspad_calibpar, m_tiltIsApplied );
		m_pix_coords_cspad = new CSPadPixCoords::PixCoordsCSPad( m_pix_coords_quad, m_cspad_calibpar, m_tiltIsApplied );
	
		//check psana's debug level
		//....so we can get rid of the verbosity when we actually run a lot of data
		MsgLogger::MsgLogLevel lvl(MsgLogger::MsgLogLevel::info);
		if ( MsgLogger::MsgLogger().logging(lvl) ) {
			cout << "------------calibration data parser---------------" << endl;
			m_cspad_calibpar->printCalibPars();
			cout << "------------pix coords 2x1-----------" << endl;
			m_pix_coords_2x1->print_member_data();
		}
	}catch(...){
		MsgLog(name, error, "Could not retrieve calibration data");
	}
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
	MsgLog(name(), info, "useShift = " << p_useShift );
	MsgLog(name(), info, "shiftX = " << p_shiftX );
	MsgLog(name(), info, "shiftY = " << p_shiftY );
	MsgLog(name(), info, "detOffset = " << p_detOffset );
	
	MsgLog(name(), info, "------CSPAD info------" );
	MsgLog(name(), info, "nRowsPerASIC = " << nRowsPerASIC );
	MsgLog(name(), info, "nColsPerASIC = " << nColsPerASIC ); 
	MsgLog(name(), info, "nRowsPer2x1 = " << nRowsPer2x1 );
	MsgLog(name(), info, "nColsPer2x1 = " << nColsPer2x1 ); 
	MsgLog(name(), info, "nMax2x1sPerQuad =  " << nMax2x1sPerQuad ); 
	MsgLog(name(), info, "nMaxQuads = " << nMaxQuads ); 
	MsgLog(name(), info, "nPxPer2x1 = " << nPxPer2x1 ); 
	MsgLog(name(), info, "nMaxPxPerQuad = " << nMaxPxPerQuad ); 
	MsgLog(name(), info, "nMaxTotalPx = " << nMaxTotalPx ); 


	MsgLog(name(), debug, "------list of stored PVs------" );
	const vector<string>& pvNames = env.epicsStore().pvNames();
	for (unsigned int i = 0; i < pvNames.size(); i++){
		MsgLog(name(), debug, pvNames.at(i) );
	}
	
	MsgLog(name(), info, "------list of read out PVs------" );
	//compile a list of general PVs that need to be read out for this job (and don't change for each event)
	vector<string> jobPVs;						// vector for list of PVs
	vector<string> jobDesc;						// vector for PV description
	
	jobPVs.push_back("CXI:DS1:MMS:06");									//# 0
	jobDesc.push_back("detector position");
	unsigned int detZ_no = 0;
	
	jobPVs.push_back("BEND:DMP1:400:BDES");								//# 1
	jobDesc.push_back("electron beam energy");
	
	jobPVs.push_back("SIOC:SYS0:ML00:AO289");							//# 2
	jobDesc.push_back("Vernier energy");
	
	jobPVs.push_back("BPMS:DMP1:199:TMIT1H");							//# 3
	jobDesc.push_back("particle N_electrons");

	jobPVs.push_back("EVNT:SYS0:1:LCLSBEAMRATE");						//# 4
	jobDesc.push_back("LCLS repetition rate");
		
	jobPVs.push_back("SIOC:SYS0:ML00:AO195");							//# 5
	jobDesc.push_back("peak current after second bunch compressor");
	
	jobPVs.push_back("SIOC:SYS0:ML00:AO820");							//# 6
	jobDesc.push_back("pulse length");
	
	jobPVs.push_back("SIOC:SYS0:ML00:AO569");							//# 7
	jobDesc.push_back("ebeam energy loss converted to photon mJ");
	
	jobPVs.push_back("SIOC:SYS0:ML00:AO580");							//# 8
	jobDesc.push_back("calculated number of photons");
	
	jobPVs.push_back("SIOC:SYS0:ML00:AO541");							//# 9
	jobDesc.push_back("photon beam energy");
	
	jobPVs.push_back("SIOC:SYS0:ML00:AO627");							//# 10
	jobDesc.push_back("photon beam energy");
	
	jobPVs.push_back("SIOC:SYS0:ML00:AO192");							//# 11
	jobDesc.push_back("wavelength in Angstrom");
	unsigned int lambda_no = 11;
	
	
	vector<double> jobVals;						// vector for corresponding values
	jobVals.assign(jobPVs.size(), 0.);			// set all values of PVs to zero initially
	vector<bool> jobPVvalid;					// keep track of the validity of this PV
	jobPVvalid.assign(jobPVs.size(), false);
	
	for (unsigned int i = 0; i < jobPVs.size(); i++){
		try{
			jobVals.at(i) = env.epicsStore().value(jobPVs.at(i));
			MsgLog(name(), info, setw(3) << "#" << i << " " 
				<< setw(26) << jobPVs.at(i) << " = " 
				<< setw(12) << jobVals.at(i) 
				<< " (" << jobDesc.at(i) << ")" );
			jobPVvalid.at(i) = true;
		}catch(...){
			MsgLog(name(), warning, "PV " << jobPVs.at(i) << " (" << jobDesc.at(i) << ") doesn't exist." );
			jobPVvalid.at(i) = false;
		}
	}
	
	MsgLog(name(), info, "------quantities derived from selected PVs------" );
	double detZ = 0; 
	if (jobPVvalid.at(detZ_no) ){
		detZ = p_detOffset + jobVals.at(detZ_no);
	}
	double lambda = 0;
	if (jobPVvalid.at(lambda_no) ){
		lambda = jobVals.at(lambda_no);
	}
	double two_k = 2*2*M_PI/lambda;								//in units of inv. Angstrom
	MsgLog(name(), info, "detZ = " << detZ << " mm");				
	MsgLog(name(), info, "2*k = " << two_k << "Å^-1" );
	MsgLog(name(), info, " ");
	
	//---read calibration information arrays and add them to the event---
	//---possibly take some of this out if not needed and memory is needed
	//
	// in microns
	shared_ptr<array1D> pixX_um_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrX_um(), nMaxTotalPx) );
	evt.put( pixX_um_sp, IDSTRING_PX_X_um);
	shared_ptr<array1D> pixY_um_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrY_um(), nMaxTotalPx) );
	evt.put( pixY_um_sp, IDSTRING_PX_Y_um);
	
	// in pixel index (integer pixel position only)
	shared_ptr<array1D> pixX_int_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrX_int(), nMaxTotalPx) );
	evt.put( pixX_int_sp, IDSTRING_PX_X_int);
	shared_ptr<array1D> pixY_int_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrY_int(), nMaxTotalPx) );
	evt.put( pixY_int_sp, IDSTRING_PX_Y_int);

	// in pix (pixel index, including fraction)
	shared_ptr<array1D> pixX_pix_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrX_pix(), nMaxTotalPx) );
	evt.put( pixX_pix_sp, IDSTRING_PX_X_pix);
	shared_ptr<array1D> pixY_pix_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrY_pix(), nMaxTotalPx) );
	evt.put( pixY_pix_sp, IDSTRING_PX_Y_pix);
	
	
	MsgLog(name(), info, "------read pixel arrays from calib data------");	
	MsgLog(name(), info, "PixCoorArrX_um  min: " << pixX_um_sp->calcMin()  << ", max: " << pixX_um_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_um  min: " << pixY_um_sp->calcMin()  << ", max: " << pixY_um_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_int min: " << pixX_int_sp->calcMin() << ", max: " << pixX_int_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_int min: " << pixY_int_sp->calcMin() << ", max: " << pixY_int_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_pix min: " << pixX_pix_sp->calcMin() << ", max: " << pixX_pix_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_pix min: " << pixY_pix_sp->calcMin() << ", max: " << pixY_pix_sp->calcMax() );
	
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
	shared_ptr<array1D> pixX_q_sp ( new array1D( pixX_um_sp.get() ) );
	shared_ptr<array1D> pixY_q_sp ( new array1D( pixX_um_sp.get() ) );
	for (unsigned int i = 0; i < pixX_q_sp->size(); i++){
		pixX_q_sp->set(i, two_k * sin( atan(pixX_um_sp->get(i)/1000.0/detZ) ) );
		pixY_q_sp->set(i, two_k * sin( atan(pixY_um_sp->get(i)/1000.0/detZ) ) );
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
}




/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
discriminate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::beginRun()" );
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
	ostringstream evtinfo;
	evtinfo <<  "evt#" << p_count << " ";
	
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
	
	double datasum = 0.;
	int pxsum = 0;
	
	std::ostringstream osst;
//	osst << "t" << eventId->time();
	osst << std::setfill('0') << std::setw(10) << p_count;
	shared_ptr<std::string> eventname_sp( new std::string(osst.str()) );
	evt.put(eventname_sp, IDSTRING_CUSTOM_EVENTNAME);
	
	//get CSPAD data from evt object (out of XTC stream)
	Pds::Src exactSrc;				// Src object after the contents of m_sourceString have been evaluated (matched)
	shared_ptr<Psana::CsPad::DataV2> data = evt.get(m_dataSourceString, "calibrated", &exactSrc);
	
	//if no calibrated data was found in the event, use the raw data from the xtc file
	//(probably the module cspad_mod.CsPadCalib wasn't executed before)
	if ( data.get() ){
		evtinfo << "pre-calibrated data ";
	}else{
		evtinfo << "non-calibrated data ";
		data = evt.get(m_dataSourceString, "", &exactSrc);
	}
	
	//create shared_ptr to an array1D object, which can then be passed to the following modules
	shared_ptr<array1D> raw1D_sp ( new array1D(nMaxTotalPx) );
	array1D* raw1D = raw1D_sp.get();
	
	if (data.get()){
		int nQuads = data->quads_shape().at(0);
		for (int q = 0; q < nQuads; ++q) {
			const Psana::CsPad::ElementV2& el = data->quads(q);
			
			const int16_t* quad_data = el.data();
			int quad_count = el.quad();				//quadrant number
			std::vector<int> quad_shape = el.data_shape();
			int actual2x1sPerQuad = quad_shape.at(0);
			int actualPxPerQuad = actual2x1sPerQuad * nPxPer2x1;
			
			//go through the whole data of the quad, copy to array1D
			for (int i = 0; i < actualPxPerQuad; i++){
				datasum += quad_data[i];
				raw1D->set( q*nMaxPxPerQuad +i, (double)quad_data[i] );
			}
			
			pxsum += actualPxPerQuad;
			MsgLog(name(), debug, "quad " << quad_count << " of " << nQuads 
				<< " has " << actual2x1sPerQuad << " 2x1s, " << actualPxPerQuad << " pixels");
		}//for all quads
	}else{
		MsgLog(name(), error, "could not get CSPAD data" );
	}
	
	double avgPerPix = datasum/(double)pxsum;
	evtinfo << "avg:" << setw(8) << avgPerPix << ", threshold delta (" 
		<< setw(8) << avgPerPix-p_lowerThreshold << "/"
		<< setw(8) << avgPerPix-p_upperThreshold << ") ";
	
	if ( avgPerPix <= p_lowerThreshold || avgPerPix >= p_upperThreshold ){
		evtinfo << " xxxxx REJECT xxxxx";

		// all downstream modules will not be called
		skip();
		
		p_skipcount++;
	}else{
		evtinfo << " --> ACCEPT (hit# " << p_hitcount << ") <--";
		
		// add the data to the event so that other modules can access it
		evt.put(raw1D_sp, IDSTRING_CSPAD_DATA);
		
		// put intensity in hit array
		p_hitInt.push_back(avgPerPix);
		
		p_hitcount++;
	}
	
	// gracefully stop analysis job
	if (p_hitcount >= p_maxHits){
		MsgLog(name(), info, "Stopping analysis job after maximum number of hits (" << p_maxHits << ") has been reached");
		stop();
	}
	
	// increment event counter
	p_count++;
	
	// output collected information...
	MsgLog(name(), info, evtinfo.str());
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

	MsgLog(name(), info, "---------------------------------------------------");
	MsgLog(name(), info, "processed events: " << p_count 
		<< ", hits: " << p_hitcount 
		<< ", skipped: " << p_skipcount 
		<< ", hitrate: " << p_hitcount/(double)p_count * 100 << "%." );
		
	arraydata *hits = new arraydata(p_hitInt);
	MsgLog(name(), info, "\n ------hit intensity histogram------\n" << hits->getHistogramASCII(30) );
	delete hits;
}



} // namespace kitty
