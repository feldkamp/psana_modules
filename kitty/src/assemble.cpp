//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class correct...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/assemble.h"

//-----------------
// C/C++ Headers --
//-----------------

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
PSANA_MODULE_FACTORY(assemble)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
assemble::assemble (const std::string& name)
  : Module(name)
  , m_src()
{
  // get the values from configuration or use defaults
  	m_src				= configStr("source",				"CxiDs1.0:Cspad.0");
	m_calibDir      	= configStr("calibDir",				"/reg/d/psdm/CXI/cxi35711/calib");
	m_typeGroupName 	= configStr("typeGroupName",		"CsPad::CalibV1");
	m_tiltIsApplied 	= config   ("tiltIsApplied",		true);								//tilt angle correction
	m_runNumber			= config   ("runNumber",     		0);
	
	p_outputPrefix	= configStr("outputPrefix", "avg_");
	p_useNormalization 	= config   ("useNormalization", 0);

	io = new arraydataIO;
	p_sum = new array1D(nMaxTotalPx);

	p_count = 0;	
	
	m_cspad_calibpar   = new PSCalib::CSPadCalibPars(m_calibDir, m_typeGroupName, m_src, m_runNumber);
	m_pix_coords_2x1   = new CSPadPixCoords::PixCoords2x1();
	m_pix_coords_quad  = new CSPadPixCoords::PixCoordsQuad( m_pix_coords_2x1,  m_cspad_calibpar, m_tiltIsApplied );
	m_pix_coords_cspad = new CSPadPixCoords::PixCoordsCSPad( m_pix_coords_quad, m_cspad_calibpar, m_tiltIsApplied );	
	
//	p_pixX = new array1D( m_pix_coords_cspad->getPixCoorArrX_um(), nMaxTotalPx);
//	p_pixX = new array1D( m_pix_coords_cspad->getPixCoorArrX_pix(), nMaxTotalPx);	

	p_pixX = new array1D( m_pix_coords_cspad->getPixCoorArrX_int(), nMaxTotalPx);
	p_pixY = new array1D( m_pix_coords_cspad->getPixCoorArrY_int(), nMaxTotalPx);
}

//--------------
// Destructor --
//--------------
assemble::~assemble ()
{
	delete io;
	delete p_sum;
	delete p_pixX;
	delete p_pixY;
	
	delete m_cspad_calibpar;  
	delete m_pix_coords_2x1;   
	delete m_pix_coords_quad;  
	delete m_pix_coords_cspad; 
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
assemble::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "assemble::beginJob()" );
	MsgLog(name(), info, "output prefix = '" << p_outputPrefix << "'" );
	MsgLog(name(), info, "use normalization = '" << p_useNormalization << "'" );
		
	shared_ptr<array1D> pixX_um_sp ( new array1D( m_pix_coords_cspad->getPixCoorArrX_um(), nMaxTotalPx) );
	evt.put( pixX_um_sp, IDSTRING_PX_X_UM);
//	p_pixX = pixX_um_sp.get();
	
	
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
assemble::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::beginRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
assemble::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
assemble::event(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::event()" );

	shared_ptr<array1D> data = evt.get(IDSTRING_CSPAD_DATA);
	if (data){
		MsgLog(name(), debug, "read event data of size " << data->size() );
		p_sum->addArrayElementwise( data.get() );	
		p_count++;	
	}else{
		MsgLog(name(), warning, "could not get CSPAD data" );
	}
}
  
  
/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
assemble::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
assemble::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
assemble::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::endJob()" );
	
	//create average out of raw sum
	p_sum->divideByValue( p_count );

	//optional normalization		
	if (p_useNormalization == 1){
		double mean = p_sum->calcAvg();
		p_sum->divideByValue( mean );
	}
	if (p_useNormalization == 2){
		double mean = p_sum->calcMax();
		p_sum->divideByValue( mean );
	}
	
	
	//output of 2D raw image (cheetah-style)
	array2D *raw2D = new array2D();
	raw2D->createRawImageCSPAD( p_sum );
	
	array2D *assembled2D = new array2D();
	assembled2D->createAssembledImageCSPAD( p_sum, p_pixX, p_pixY );
	
	io->writeToEDF( p_outputPrefix+"_1D.edf", p_sum );
	io->writeToEDF( p_outputPrefix+"_raw2D.edf", raw2D );
	io->writeToEDF( p_outputPrefix+"_asm2D.edf", assembled2D );

	delete raw2D;
	delete assembled2D;


	MsgLog(name(), info, "---complete histogram of averaged data---\n" << p_sum->getHistogramASCII(50) );
}



} // namespace kitty
