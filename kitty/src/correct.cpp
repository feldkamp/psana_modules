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
#include "kitty/correct.h"

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
PSANA_MODULE_FACTORY(correct)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
correct::correct (const std::string& name)
  : Module(name)
{
  // get the values from configuration or use defaults
	p_back_fn		= configStr("background", "");	
	p_useBack		= config   ("useBackground", 0);
		
	io = new arraydataIO;
	p_sum = new array1D(nMaxTotalPx);
	p_back = new array1D();
}

//--------------
// Destructor --
//--------------
correct::~correct ()
{
	delete io;
	delete p_sum;
	delete p_back;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
correct::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correct::beginJob()" );
	MsgLog(name(), info, "background file = '" << p_back_fn << "'" );
	MsgLog(name(), info, "use background correction = '" << p_useBack << "'" );
	
	//read background, if a file was specified
	if (p_useBack){
		if (p_back_fn != ""){
			array1D *p_back = new array1D();
			int fail = io->readFromEDF( p_back_fn, p_back );
			if (fail){
				MsgLog(name(), warning, "Could not read background, continuing without background subtraction!");
				p_useBack = 0;	
			}
		}else{
				MsgLog(name(), warning, "No background file specified in config file, continuing without background subtraction!");
				p_useBack = 0;			
		}
	}
		
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
correct::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correct::beginRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
correct::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correct::beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
correct::event(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correct::event()" );

	shared_ptr<array1D> data = evt.get();
	if (data){
		MsgLog(name(), debug, "read event data of size " << data->size() );

		if (p_useBack){
			data->subtractArrayElementwise( p_back );
		}
	}else{
		MsgLog(name(), info, "could not get CSPAD data" );
	}

	p_sum->addArrayElementwise( data.get() );	
	p_count++;
}
  
  
/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
correct::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correct::endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
correct::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correct::endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
correct::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correct::endJob()" );
	
	//create average out of raw sum
	p_sum->divideByValue( p_count );
	
	//output of 2D raw image (cheetah-style)
	array2D *raw2D = new array2D();
	assembleRawImageCSPAD( p_sum, raw2D );
	io->writeToEDF("rawsum_avg_1D.edf", p_sum);
	io->writeToEDF("rawsum_avg_2D.edf", raw2D);

//DEBUG!!!
	for (int i=0; i<10; i++){
		cout << "1D(" << i << ") = " << p_sum->get_atIndex(i) << endl;
		cout << "2D(" << i << ") = " << raw2D->get_atIndex(i) << endl;
	}	
	delete raw2D;
	
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
correct::assembleRawImageCSPAD( array1D *input, array2D *&output ){
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



} // namespace kitty
