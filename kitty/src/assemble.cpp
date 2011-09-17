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
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
assemble::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::beginJob()" );
	MsgLog(name(), info, "output prefix = '" << p_outputPrefix << "'" );

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

	shared_ptr<array1D> data = evt.get();
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
		//create average out of raw sum
	p_sum->divideByValue( p_count );
	
	//output of 2D raw image (cheetah-style)
	array2D *raw2D = new array2D();
	createRawImageCSPAD( p_sum, raw2D );
	
	array2D *assembled2D = new array2D();
	createAssembledImageCSPAD( p_sum, assembled2D );
	
	io->writeToEDF( p_outputPrefix+"_1D.edf", p_sum );
	io->writeToEDF( p_outputPrefix+"_raw2D.edf", raw2D );
	io->writeToEDF( p_outputPrefix+"_asm2D.edf", assembled2D );

	delete raw2D;
	delete assembled2D;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
assemble::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "assemble::endJob()" );
	
	//create average out of raw sum
	p_sum->divideByValue( p_count );
	
	//output of 2D raw image (cheetah-style)
	array2D *raw2D = new array2D();
	createRawImageCSPAD( p_sum, raw2D );
	
	array2D *assembled2D = new array2D();
	createAssembledImageCSPAD( p_sum, assembled2D );
	
	io->writeToEDF( p_outputPrefix+"_1D.edf", p_sum );
	io->writeToEDF( p_outputPrefix+"_raw2D.edf", raw2D );
	io->writeToEDF( p_outputPrefix+"_asm2D.edf", assembled2D );

	delete raw2D;
	delete assembled2D;
}


//------------------------------------------------------------- assembleRawImageCSPAD
// function to create 'raw' CSPAD images from plain 1D data, where
// dim1 : 388 : rows of a 2x1
// dim2 : 185 : columns of a 2x1
// dim3 :   8 : 2x1 sections in a quadrant (align as super-columns)
// dim4 :   4 : quadrants (align as super-rows)
//
//    +--+--+--+--+--+--+--+--+
// q0 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
// q1 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
// q2 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
// q3 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
//     s0 s1 s2 s3 s4 s5 s6 s7
void
assemble::createRawImageCSPAD( array1D *input, array2D *&output ){
	delete output;
	output = new array2D( nMaxQuads*nRowsPer2x1, nMax2x1sPerQuad*nColsPer2x1 );
	
	//sort into ordered 2D data
	for (int q = 0; q < nMaxQuads; q++){
		int superrow = q*nRowsPer2x1;
		for (int s = 0; s < nMax2x1sPerQuad; s++){
			int supercol = s*nColsPer2x1;
			for (int c = 0; c < nColsPer2x1; c++){
				for (int r = 0; r < nRowsPer2x1; r++){
					output->set( superrow+r, supercol+c, 
						input->get( q*nMaxPxPerQuad + s*nPxPer2x1 + c*nRowsPer2x1 + r ) );
				}
			}
		}
	}
	
	//transpose to be conform with cheetah's convention
	output->transpose();
}


void 
assemble::createAssembledImageCSPAD( array1D *input, array2D *&output ){
	int NX_CSPAD = 1750;
	int NY_CSPAD = 1750;
	delete output;
	output = new array2D( NY_CSPAD, NX_CSPAD );

	//sort into ordered 2D data
	for (int q = 0; q < nMaxQuads; q++){
		for (int s = 0; s < nMax2x1sPerQuad; s++){
			for (int c = 0; c < nColsPer2x1; c++){
				for (int r = 0; r < nRowsPer2x1; r++){
					int index = q*nMaxPxPerQuad + s*nPxPer2x1 + c*nRowsPer2x1 + r;
					output->set( (int)p_pixY->get(index), (int)p_pixX->get(index), input->get(index) );
				}
			}
		}
	}	
}


} // namespace kitty
