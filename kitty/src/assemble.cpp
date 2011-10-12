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
#include "kitty/util.h"


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
	, p_outputPrefix("")
	, p_useNormalization(0)
	, io(0)
	, p_sum(0)
	, p_pixX_sp()
	, p_pixY_sp()
	, p_tifOut(0)
	, p_edfOut(0)
	, p_h5Out(0)
	, p_singleOutput(0)
	, p_count(0)
{	
	p_outputPrefix			= configStr("outputPrefix", 		"img");
	p_tifOut 				= config   ("tifOut", 				0);
	p_edfOut 				= config   ("edfOut", 				1);
	p_h5Out 				= config   ("h5Out", 				0);
	p_singleOutput			= config   ("singleOutput",			0);
	p_useNormalization 		= config   ("useNormalization", 	0);
	
	p_sum = new array1D(nMaxTotalPx);
	io = new arraydataIO();
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
	MsgLog(name(), info, "tifOut            = '" << p_tifOut << "'" );
	MsgLog(name(), info, "edfOut            = '" << p_edfOut << "'" );
	MsgLog(name(), info, "h5Out             = '" << p_h5Out << "'" );
	MsgLog(name(), info, "singleOutput      = '" << p_singleOutput << "'" );
	MsgLog(name(), info, "use normalization = '" << p_useNormalization << "'" );

//	p_pixX = ( (shared_ptr<array1D>) evt.get(IDSTRING_PX_X_int) ).get();
//	p_pixY = ( (shared_ptr<array1D>) evt.get(IDSTRING_PX_Y_int) ).get();
	p_pixX_sp = evt.get(IDSTRING_PX_X_int); 
	p_pixY_sp = evt.get(IDSTRING_PX_Y_int); 

	if (p_pixX_sp && p_pixY_sp){
		MsgLog(name(), info, "read data for pixX(n=" << p_pixX_sp->size() << "), pixY(n=" << p_pixY_sp->size() << ")" );
	}else{
		MsgLog(name(), warning, "could not get data from pixX(addr=" << p_pixX_sp.get() << ") or pixY(addr=" << p_pixY_sp.get() << ")" );
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

	shared_ptr<array1D> data_sp = evt.get(IDSTRING_CSPAD_DATA);
	string eventname_str = *( (shared_ptr<string>) evt.get(IDSTRING_CUSTOM_EVENTNAME) ).get();
	
	if (data_sp){
		MsgLog(name(), debug, "read event data of size " << data_sp->size() );
		p_sum->addArrayElementwise( data_sp.get() );	
		p_count++;	
		
		if ( p_singleOutput ){

			array2D *assembled2D = 0;
			int fail_asm = createAssembledImageCSPAD( p_sum, p_pixX_sp.get(), p_pixY_sp.get(), assembled2D );
			if ( !(p_count % p_singleOutput) ){
				if (p_edfOut){
					if (!fail_asm) io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_asm2D.edf", assembled2D );
				}
				if (p_h5Out){
					if (!fail_asm) io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_asm2D.h5", assembled2D );
				}
				if (p_tifOut){
					if (!fail_asm) io->writeToTiff( p_outputPrefix+"_evt"+eventname_str+"_asm2D.tif", assembled2D );
				}
			}//if modulo 
			delete assembled2D;

		}//if singleOutput
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
		double max = p_sum->calcMax();
		p_sum->divideByValue( max );
	}

	//assemble ASICs
	array2D *assembled2D = 0;
	int fail_asm = createAssembledImageCSPAD( p_sum, p_pixX_sp.get(), p_pixY_sp.get(), assembled2D );
		
	//output of 2D raw image (cheetah-style)
	array2D *raw2D = 0;
	int fail_raw = createRawImageCSPAD( p_sum, raw2D );
	
	string ext = "";
	if (p_edfOut){
		ext = ".edf";
		io->writeToEDF( p_outputPrefix+"_avg_1D.edf", p_sum );
		if (!fail_raw) io->writeToEDF( p_outputPrefix+"_avg_raw2D"+ext, raw2D );
		if (!fail_asm) io->writeToEDF( p_outputPrefix+"_avg_asm2D"+ext, assembled2D );
	}
	if (p_h5Out){
		ext = ".h5";
		//io->writeToHDF5( p_outputPrefix+"_avg_1D"+ext, p_sum );
		if (!fail_raw) io->writeToHDF5( p_outputPrefix+"_avg_raw2D"+ext, raw2D );
		if (!fail_asm) io->writeToHDF5( p_outputPrefix+"_avg_asm2D"+ext, assembled2D );
	}
	if (p_tifOut){
		ext = ".tif";
		//io->writeToTiff( p_outputPrefix+"_avg_1D"+ext, p_sum );
		if (!fail_raw) io->writeToTiff( p_outputPrefix+"_avg_raw2D"+ext, raw2D );
		if (!fail_asm) io->writeToTiff( p_outputPrefix+"_avg_asm2D"+ext, assembled2D );
	}
	delete raw2D;
	delete assembled2D;
	
	MsgLog(name(), debug, "---complete histogram of averaged data---\n" << p_sum->getHistogramASCII(50) );
}



} // namespace kitty
