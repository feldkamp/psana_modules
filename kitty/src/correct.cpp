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
	p_gain_fn		= configStr("gainmap", "");	
	p_useGain		= config   ("useGainmap", 0);
		
	io = new arraydataIO;
	p_back = new array1D();
	p_gain = new array1D();
	
	p_count = 0;
}

//--------------
// Destructor --
//--------------
correct::~correct ()
{
	delete io;
	delete p_back;
	delete p_gain;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
correct::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "correct::beginJob()" );
	MsgLog(name(), info, "background file = '" << p_back_fn << "'" );
	MsgLog(name(), info, "use background correction = '" << p_useBack << "'" );
	MsgLog(name(), info, "gain file = '" << p_gain_fn << "'" );
	MsgLog(name(), info, "use gain correction = '" << p_useGain << "'" );
	
	//read background, if a file was specified
	if (p_useBack){
		if (p_back_fn != ""){
			p_back = new array1D;
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

	//read gainmap, if a file was specified
	if (p_useGain){
		if (p_gain_fn != ""){
			p_gain = new array1D;
			int fail = io->readFromEDF( p_gain_fn, p_gain );
			if (fail){
				MsgLog(name(), warning, "Could not read gainmap, continuing without gain correction!");
				p_useGain = 0;	
			}
		}else{
				MsgLog(name(), warning, "No gain file specified in config file, continuing without gain correction!");
				p_useGain = 0;			
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
		
		if (p_useGain){
			data->divideByArrayElementwise( p_gain );
		}
	
		p_count++;	
	}else{
		MsgLog(name(), warning, "could not get CSPAD data" );
	}
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
}



} // namespace kitty
