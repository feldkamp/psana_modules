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
{	
	p_outputPrefix			= configStr("outputPrefix", 		"avg_");
	p_useNormalization 		= config   ("useNormalization", 	0);

	io = new arraydataIO;
	p_sum = new array1D(nMaxTotalPx);

	p_count = 0;	
}

//--------------
// Destructor --
//--------------
assemble::~assemble ()
{
	delete io;
	delete p_sum;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
assemble::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "assemble::beginJob()" );
	MsgLog(name(), info, "output prefix = '" << p_outputPrefix << "'" );
	MsgLog(name(), info, "use normalization = '" << p_useNormalization << "'" );
	
	p_pixX_sp = evt.get(IDSTRING_PX_X_int);
	p_pixY_sp = evt.get(IDSTRING_PX_Y_int);
	if (p_pixX_sp && p_pixY_sp){
		MsgLog(name(), info, "read data for pixX(n=" << p_pixX_sp->size() << "), pixY(n=" << p_pixY_sp->size() << ")" );
	}else{
		MsgLog(name(), warning, "could not get data from pixX(addr=" << p_pixX_sp << ") or pixY(addr=" << p_pixY_sp << ")" );
	}
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
	io->writeToEDF( p_outputPrefix+"_1D.edf", p_sum );
	io->writeToEDF( p_outputPrefix+"_raw2D.edf", raw2D );
	delete raw2D;
	
	if (p_pixX_sp && p_pixY_sp){
		array2D *assembled2D = new array2D();
		assembled2D->createAssembledImageCSPAD( p_sum, p_pixX_sp.get(), p_pixY_sp.get() );
		io->writeToEDF( p_outputPrefix+"_asm2D.edf", assembled2D );
		delete assembled2D;

		MsgLog(name(), info, "---complete histogram of averaged data---\n" << p_sum->getHistogramASCII(50) );
	}else{
		MsgLog(name(), warning, "could not get data from p_pixX(n=" << p_pixX_sp << ") or p_pixY(addr=" << p_pixY_sp << ")" );
	}
}



} // namespace kitty
