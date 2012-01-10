//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class discriminate...
//
// Author List:
//      Jan Moritz Feldkamp
//		Jonas Alexander Sellberg
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
using ns_cspad_util::create1DFromRawImageCSPAD;
using ns_cspad_util::createAssembledImageCSPAD;
using ns_cspad_util::createRawImageCSPAD;


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
	, p_lowerThreshold(0.)
	, p_upperThreshold(0.)
	, p_discriminateAlogrithm(0)
	, p_outputPrefix("")
	, p_pixelVectorOutput(0)
	, p_maxHits(0)
	, p_skipcount(0)
	, p_hitcount(0)
	, p_count(0)
	, p_makePixelArrays_count(0)
	, p_stopflag(false)
	, p_useShift(0)	
	, p_shiftX(0.)
	, p_shiftY(0.)
	, p_detOffset(0.)
	, p_detDistance(0.)
	, p_lambda(0.)
	, p_criticalPVchange(false)
	, p_pvs()
	, p_sum_sp()
	, p_hitInt()
	, p_pixX_um_sp()
	, p_pixY_um_sp()
	, p_pixX_int_sp()
	, p_pixY_int_sp()
	, p_pixX_pix_sp()
	, p_pixY_pix_sp()
	, p_pixX_q_sp()
	, p_pixY_q_sp()
	, p_pixTwoTheta_sp()
	, p_pixPhi_sp()
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
	
	p_lowerThreshold 			= config("lowerThreshold", 			-10000000.0);
	p_upperThreshold 			= config("upperThreshold", 			10000000.0);
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
	MsgLog(name(), debug, "beginJob()" );
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
	
	
	
	p_pvs[pvDetPos].name = "CXI:DS1:MMS:06";
	p_pvs[pvDetPos].desc = "detector position";

	p_pvs[pvLambda].name = "SIOC:SYS0:ML00:AO192";
	p_pvs[pvLambda].desc = "wavelength [nm]";
		
	p_pvs[pvElEnergy].name = "BEND:DMP1:400:BDES";
	p_pvs[pvElEnergy].desc = "electron beam energy";
	
	p_pvs[pvNElectrons].name = "BPMS:DMP1:199:TMIT1H";
	p_pvs[pvNElectrons].desc = "particle N_electrons";

	p_pvs[pvRepRate].name = "EVNT:SYS0:1:LCLSBEAMRATE";
	p_pvs[pvRepRate].desc = "LCLS repetition rate [Hz]";
		
	p_pvs[pvPeakCurrent].name = "SIOC:SYS0:ML00:AO195";
	p_pvs[pvPeakCurrent].desc = "peak current after second bunch compressor";
	
	p_pvs[pvPulseLength].name = "SIOC:SYS0:ML00:AO820";
	p_pvs[pvPulseLength].desc = "pulse length";
	
	p_pvs[pvEbeamLoss].name = "SIOC:SYS0:ML00:AO569";
	p_pvs[pvEbeamLoss].desc = "ebeam energy loss converted to photon mJ";
	
	p_pvs[pvNumPhotons].name = "SIOC:SYS0:ML00:AO580";
	p_pvs[pvNumPhotons].desc = "calculated number of photons";
	
	p_pvs[pvPhotonEnergy].name = "SIOC:SYS0:ML00:AO541";
	p_pvs[pvPhotonEnergy].desc = "photon beam energy [eV]";
	
	//initialize the value and valid fields of the pv structs in the map
	for (map<string,pv>::iterator it = p_pvs.begin(); it != p_pvs.end(); it++){
		it->second.value = 0.;
		it->second.valid = false;
		it->second.changed = false;
	} 

}




/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
discriminate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "beginRun()" );
	
	//put output string into event to make it accessible for all modules
	shared_ptr<std::string> outputPrefix_sp( new std::string(p_outputPrefix) );
	evt.put(outputPrefix_sp, IDSTRING_OUTPUT_PREFIX);

	MsgLog(name(), debug, "------list of stored PVs------" );
	const vector<string>& pvNames = env.epicsStore().pvNames();
	for (unsigned int i = 0; i < pvNames.size(); i++){
		MsgLog(name(), debug, pvNames.at(i) );
	}
	
	
	//try to get the PV values
	map<string,pv>::iterator it;
	ostringstream osst;
	for (it = p_pvs.begin(); it != p_pvs.end(); it++){
		osst << setw(20) << it->first << " " << setw(26) << it->second.name; 
		try{
			it->second.value = env.epicsStore().value(it->second.name);
			it->second.valid = true;
			osst << " = " << setw(12) << it->second.value;
		}catch(...){
			it->second.valid = false;
			osst << setw(12) << " ---- ";
		}
		osst << " (" << it->second.desc << ")" << endl;
	}
	MsgLog(name(), info, "------list of read out PVs------\n" << osst.str() );
	
	
//	std::list<EventKey> keys = evt.keys();
//	MsgLog(name(), info, "------" << keys.size() << " keys in event------");
//	for ( list<EventKey>::iterator it = keys.begin(); it != keys.end(); it++ ){
//		ostringstream out;
//		it->print(out);
//		MsgLog(name(), info, out.str() << ", key: '" << it->key() << "'");
//	}
	

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
	
	makePixelArrays();
	addPixelArraysToEvent( evt );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
discriminate::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "beginCalibCycle()" );
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
	MsgLog(name(), trace, "event ID: " << *eventId);
	
	ostringstream evtinfo;
	evtinfo <<  "evt #" << p_count << ", ";
	
	//check, if relevant event PVs have changed
	for (map<string,pv>::iterator it = p_pvs.begin(); it != p_pvs.end(); it++){
		double prev_val = it->second.value;
		try{
			it->second.value = env.epicsStore().value(it->second.name);
			it->second.valid = true;
		}catch(...){
			it->second.valid = false;
		}
		
		if (it->second.value != prev_val){
			MsgLog(name(), trace, "PV value changed in " << it->first 
				<< ", from " << prev_val << " to " << it->second.value );
			it->second.changed = true;
		}else{
			it->second.changed = false;
		}
	}
	
	if(p_pvs[pvDetPos].changed || p_pvs[pvLambda].changed){
		//recalculate the pixel vectors
		MsgLog(name(), info, "PV for detector position or wavelength changed in this event");
		MsgLog(name(), info, "\n"
			<< "\n=================================="
			<< "\n=== RECALCULATING PIXEL ARRAYS ==="
			<< "\n==================================" 
			<< "\n");
		makePixelArrays();
		p_criticalPVchange = true;
	}
	
	//append criticalPVchange flag to event to communicate this to other modules
	//once set to true, p_criticalPVchange needs to stay true at least until the next hit
	//so that subsequent modules have the chance to act on the change in their event()
	shared_ptr<bool> updatedPV_sp( new bool(false) );
	evt.put( updatedPV_sp, IDSTRING_PV_CHANGED );
	
	//put pixel arrays into event, so that other modules can use them in their event()
	addPixelArraysToEvent( evt );
		
	std::ostringstream osst;
//	osst << "t" << eventId->time();
	osst << std::setfill('0') << std::setw(10) << p_count;
	shared_ptr<std::string> eventname_sp( new std::string(osst.str()) );
	evt.put( eventname_sp, IDSTRING_CUSTOM_EVENTNAME);
	
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
			
			//this version works for ana-0.3.x releases
			//more recent releases use the ndarray version below
			/*
			const int16_t* quad_data = el.data();
			std::vector<int> quad_shape = el.data_shape();
			int actual2x1sPerQuad = quad_shape.at(0);
			int actualPxPerQuad = actual2x1sPerQuad * nPxPer2x1;
			for (int i = 0; i < actualPxPerQuad; i++){
				raw1D->set( q*nMaxPxPerQuad +i, (double)quad_data[i] );
			}
			*/
			
			//this version works for ana-0.4.0 and up
			//if you get compile errors here, update your release directory by calling "relupgrade ana-0.4.0"
			//3d data structure for each quad
			//shape of quad_data for CSPAD is (sections, rows, columns) = (8, 388, 185)
			ndarray<int16_t, 3> quad_data = el.data();
			const unsigned int *quad_shape = quad_data.shape();
			int sections = quad_shape[0];
			int rows = quad_shape[1];
			int cols = quad_shape[2];
			int actualPxPerQuad = sections * rows * cols;
			
			//go through the whole data of the quad, copy to array1D
			for (int i = 0; i < actualPxPerQuad; i++){
				raw1D->set( q*nMaxPxPerQuad +i, (double) (quad_data.data()[i]) );
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
	
	ios_base::fmtflags flags = evtinfo.flags();
	evtinfo << right  
			<< "avg:" << thresholdingAvg 
			<< ", threshold delta (" << setprecision(3) << showpos 
			<< thresholdingAvg-p_lowerThreshold << "/"
			<< thresholdingAvg-p_upperThreshold << ") ";
	evtinfo.flags(flags);
			
			
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
		
		if (p_criticalPVchange){
			*updatedPV_sp = true;				// tell other modules to update their PV-dependent properties
			p_criticalPVchange = false;			// reset the module-internal flag
		}
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
	MsgLog(name(), debug,  "endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
discriminate::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
discriminate::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endJob()" );

	//create average out of raw sum
	p_sum_sp->divideByValue( p_hitcount );

	MsgLog(name(), info, "---------------------------------------------------");
	MsgLog(name(), info, "processed events: " << p_count 
		<< ", hits: " << p_hitcount 
		<< ", skipped: " << p_skipcount 
		<< ", hitrate: " << p_hitcount/(double)p_count * 100 << "%." );
	
	if (p_makePixelArrays_count > 1){
		MsgLog(name(), info, "pixel arrays had to be recalculated " << p_makePixelArrays_count-1 << " times.");
	}
	
	MsgLog(name(), trace, "---complete histogram of averaged data---\n" << p_sum_sp->getHistogramASCII(50) );
		
	arraydata *hits = new arraydata(p_hitInt);
	MsgLog(name(), info, "\n ------hit intensity histogram------\n" << hits->getHistogramASCII(30) );
	delete hits;
}




/// ------------------------------------------------------------------------------------------------
void
discriminate::makePixelArrays(){
	MsgLog(name(), info, "Calculating pixel position arrays");

	//determine detector distance from p_detOffset and the PV value for the detector stage
	if (p_pvs[pvDetPos].valid){
		p_detDistance = p_detOffset + p_pvs[pvDetPos].value;
	}else{
		MsgLog(name(), error, "could not get PV for det pos");
	}
	
	//get the wavelength from PV
	double k = 0.;
	if (p_pvs[pvLambda].valid ){
		p_lambda = p_pvs[pvLambda].value;
		k = 2*M_PI/p_lambda;								//in units of inv. nanometers
	}else{
		MsgLog(name(), error, "could not get PV for lambda");
	}
	
	MsgLog(name(), info, "------ PV-derived quantities ------" );
	MsgLog(name(), info, "detDistance = " << p_detDistance << " mm");				
	MsgLog(name(), info, "k = " << k << " nm^-1");

	/*---read calibration information arrays---
	 *---possibly take some of this out if not needed and memory is needed
	 *
	 * IMPORTANT! After correspondence with Mikhail Dubrovin, it is
	 * clear that in the class CSPadPixCoords, the coordinate system is adapted
	 * so that it is the same as used in many graphical applications 
	 * (e.g. python's matplotlib.imshow) that plots 2D matrices. This means that:
	 * - the 1st ascending row-index is for the X-coordinate, and is
	 *   accessed through getPixCoorArrX()
	 * - the 2nd ascending column-index is for the Y-coordinate, and is
	 *   accessed through getPixCoorArrY()
	 * - the origin of this matrix coordinate system is located in the 
	 *	 top left corner, as it should be for matrix element a(1,1)
	 * - the X-axis is directed downward from a(1,1) to a(n,1)
	 * - the Y-axis is directed to the right from a(1,1) to a(1,n)
	 *
	 *		=> getPixCoorArrX() corresponds to the (-Y)-axis in the CXI coordinate system
	 *
	 *		=> getPixCoorArrY() corresponds to the (+X)-axis in the CXI coordinate system
	 */
	// in microns
	p_pixX_um_sp = shared_ptr<array1D>( new array1D( m_pix_coords_cspad->getPixCoorArrY_um(), nMaxTotalPx) );
	p_pixY_um_sp = shared_ptr<array1D> ( new array1D( m_pix_coords_cspad->getPixCoorArrX_um(), nMaxTotalPx) );
	
	// in pixel index (integer pixel position only)
	p_pixX_int_sp = shared_ptr<array1D>( new array1D( m_pix_coords_cspad->getPixCoorArrY_int(), nMaxTotalPx) );
	p_pixY_int_sp = shared_ptr<array1D>( new array1D( m_pix_coords_cspad->getPixCoorArrX_int(), nMaxTotalPx) );
	
	// in pix (pixel index, including fraction)
	p_pixX_pix_sp = shared_ptr<array1D>( new array1D( m_pix_coords_cspad->getPixCoorArrY_pix(), nMaxTotalPx) );
	p_pixY_pix_sp = shared_ptr<array1D>( new array1D( m_pix_coords_cspad->getPixCoorArrX_pix(), nMaxTotalPx) );

	// q-value vectors (inverse nanometers)
	p_pixX_q_sp = shared_ptr<array1D>( new array1D(nMaxTotalPx) );
	p_pixY_q_sp = shared_ptr<array1D>( new array1D(nMaxTotalPx) );
	
	// angle vectors (radians)
	p_pixTwoTheta_sp = shared_ptr<array1D>( new array1D(nMaxTotalPx) );
	p_pixPhi_sp = shared_ptr<array1D>( new array1D(nMaxTotalPx) );	
	
					
	MsgLog(name(), debug, "------read pixel arrays from calib data------");	
	MsgLog(name(), debug, "PixCoorArrX_um  min: " << p_pixX_um_sp->calcMin()  << ", max: " << p_pixX_um_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrY_um  min: " << p_pixY_um_sp->calcMin()  << ", max: " << p_pixY_um_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrX_int min: " << p_pixX_int_sp->calcMin() << ", max: " << p_pixX_int_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrY_int min: " << p_pixY_int_sp->calcMin() << ", max: " << p_pixY_int_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrX_pix min: " << p_pixX_pix_sp->calcMin() << ", max: " << p_pixX_pix_sp->calcMax() );
	MsgLog(name(), debug, "PixCoorArrY_pix min: " << p_pixY_pix_sp->calcMin() << ", max: " << p_pixY_pix_sp->calcMax() );
	
	//shift pixel arrays to be centered around incoming beam at (0, 0)
	//this only works exactly, when the beam was exactly centered in the CSPAD
	//otherwise, use manual correction of shiftX & shiftY below
	double shift_X_um = (p_pixX_um_sp->calcMin()+p_pixX_um_sp->calcMax())/2;
	double shift_Y_um = (p_pixY_um_sp->calcMin()+p_pixY_um_sp->calcMax())/2;
	double shift_X_int = (p_pixX_int_sp->calcMin()+p_pixX_int_sp->calcMax())/2;
	double shift_Y_int = (p_pixY_int_sp->calcMin()+p_pixY_int_sp->calcMax())/2;
	double shift_X_pix = (p_pixX_pix_sp->calcMin()+p_pixX_pix_sp->calcMax())/2;
	double shift_Y_pix = (p_pixY_pix_sp->calcMin()+p_pixY_pix_sp->calcMax())/2;
	
	if (p_useShift){
		//pixel size values estimated from the read out arrays above, seems about right
		const double pixelSizeXY_um = m_cspad_calibpar->getColSize_um();		// == getRowSize_um(), square pixels (luckily)
		shift_X_um = p_shiftX * pixelSizeXY_um;
		shift_Y_um = p_shiftY * pixelSizeXY_um;
		shift_X_int = p_shiftX;
		shift_Y_int = p_shiftY;
		shift_X_pix = p_shiftX;
		shift_Y_pix = p_shiftY;
	}
	
	p_pixX_um_sp->subtractValue( shift_X_um );
	p_pixY_um_sp->subtractValue( shift_Y_um );
	p_pixX_int_sp->subtractValue( shift_X_int );
	p_pixY_int_sp->subtractValue( shift_Y_int );
	p_pixX_pix_sp->subtractValue( shift_X_pix );
	p_pixY_pix_sp->subtractValue( shift_Y_pix );
	
	p_pixY_um_sp->multiplyByValue( -1.0 );
	p_pixY_int_sp->multiplyByValue( -1.0 );
	p_pixY_pix_sp->multiplyByValue( -1.0 );
	
	
	//fill q-vectors		
	for (unsigned int i = 0; i < p_pixX_q_sp->size(); i++){
		double x_um = p_pixX_um_sp->get(i);
		double y_um = p_pixY_um_sp->get(i);
		double r_um = sqrt( x_um*x_um + y_um*y_um );
		double twoTheta = atan( r_um/1000.0/p_detDistance );
		double phi = atan2( y_um, x_um );
		if (phi < 0) { // make sure the angle is between 0 and 2PI
			phi += 2*M_PI;
		}
		p_pixTwoTheta_sp->set(i, twoTheta);
		p_pixPhi_sp->set(i, phi);
		p_pixX_q_sp->set(i, 2*k * sin(twoTheta/2) * cos(phi) );
		p_pixY_q_sp->set(i, 2*k * sin(twoTheta/2) * sin(phi) );
	}
	
	MsgLog(name(), info, "------shifted pixels arrays------");	
	MsgLog(name(), info, "PixCoorArrX_um  min: " << p_pixX_um_sp->calcMin()  << ", max: " << p_pixX_um_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_um  min: " << p_pixY_um_sp->calcMin()  << ", max: " << p_pixY_um_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_int min: " << p_pixX_int_sp->calcMin() << ", max: " << p_pixX_int_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_int min: " << p_pixY_int_sp->calcMin() << ", max: " << p_pixY_int_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_pix min: " << p_pixX_pix_sp->calcMin() << ", max: " << p_pixX_pix_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_pix min: " << p_pixY_pix_sp->calcMin() << ", max: " << p_pixY_pix_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrX_q   min: " << p_pixX_q_sp->calcMin()   << ", max: " << p_pixX_q_sp->calcMax() );
	MsgLog(name(), info, "PixCoorArrY_q   min: " << p_pixY_q_sp->calcMin()   << ", max: " << p_pixY_q_sp->calcMax() );
	MsgLog(name(), info, "PixTwoTheta     min: " << p_pixTwoTheta_sp->calcMin()   << ", max: " << p_pixTwoTheta_sp->calcMax() );
	MsgLog(name(), info, "PixPhi          min: " << p_pixPhi_sp->calcMin()   << ", max: " << p_pixPhi_sp->calcMax() );
	
	if (p_pixelVectorOutput){
		arraydataIO *io = new arraydataIO();
		string ext = ".h5";
		array2D *two = 0;
		
		//raw images
		ext = "_raw.h5";
		
		createRawImageCSPAD( p_pixX_um_sp.get(), two );		
		io->writeToFile( p_outputPrefix+"_pixX_um"+ext, two );
		
		createRawImageCSPAD( p_pixY_um_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_um"+ext, two );
		
		createRawImageCSPAD( p_pixX_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_int"+ext, two );
		
		createRawImageCSPAD( p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_int"+ext, two );
		
		createRawImageCSPAD( p_pixX_pix_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_p_pix"+ext, two );
		
		createRawImageCSPAD( p_pixY_pix_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_pix"+ext, two );
		
		createRawImageCSPAD( p_pixX_q_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_q"+ext, two );
		
		createRawImageCSPAD( p_pixY_q_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_q"+ext, two );
		
		createRawImageCSPAD( p_pixTwoTheta_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixTwoTheta"+ext, two );
		
		createRawImageCSPAD( p_pixPhi_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixPhi"+ext, two );
		
		//assembled images
		ext = "_asm.h5";

		createAssembledImageCSPAD( p_pixX_um_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );		
		io->writeToFile( p_outputPrefix+"_pixX_um"+ext, two );
		
		createAssembledImageCSPAD( p_pixY_um_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_um"+ext, two );
		
		createAssembledImageCSPAD( p_pixX_int_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_int"+ext, two );
		
		createAssembledImageCSPAD( p_pixY_int_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_int"+ext, two );
		
		createAssembledImageCSPAD( p_pixX_pix_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_pix"+ext, two );
		
		createAssembledImageCSPAD( p_pixY_pix_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_pix"+ext, two );
		
		createAssembledImageCSPAD( p_pixX_q_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixX_q"+ext, two );
		
		createAssembledImageCSPAD( p_pixY_q_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixY_q"+ext, two );
		
		createAssembledImageCSPAD( p_pixTwoTheta_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixTwoTheta"+ext, two );
		
		createAssembledImageCSPAD( p_pixPhi_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), two );
		io->writeToFile( p_outputPrefix+"_pixPhi"+ext, two );
		
		delete two;
		delete io;
	}//if vector output
	
	p_makePixelArrays_count++;
}

void	
discriminate::addPixelArraysToEvent( Event &evt ){
	evt.put( p_pixX_um_sp, IDSTRING_PX_X_um);
	evt.put( p_pixY_um_sp, IDSTRING_PX_Y_um);
	evt.put( p_pixX_int_sp, IDSTRING_PX_X_int);
	evt.put( p_pixY_int_sp, IDSTRING_PX_Y_int);
	evt.put( p_pixX_pix_sp, IDSTRING_PX_X_pix);
	evt.put( p_pixY_pix_sp, IDSTRING_PX_Y_pix);
	evt.put( p_pixX_q_sp, IDSTRING_PX_X_q);
	evt.put( p_pixY_q_sp, IDSTRING_PX_Y_q);
	evt.put( p_pixTwoTheta_sp, IDSTRING_PX_TWOTHETA);
	evt.put( p_pixPhi_sp, IDSTRING_PX_PHI);	
}	


} // namespace kitty
