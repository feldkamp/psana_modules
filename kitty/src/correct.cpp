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
	p_back_fn			= configStr("background", 		"");	
	p_useBack			= config   ("useBackground", 	0);
	p_gain_fn			= configStr("gainmap", 			"");	
	p_useGain			= config   ("useGainmap", 		0);
	p_mask_fn			= configStr("mask", 			"");	
	p_useMask			= config   ("useMask", 			0);
		
	io = new arraydataIO;
	p_back = new array1D();
	p_gain = new array1D();
	p_mask = new array1D();
	
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
	delete p_mask;
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
	MsgLog(name(), info, "mask file = '" << p_mask_fn << "'" );
	MsgLog(name(), info, "use mask correction = '" << p_useMask << "'" );

		
	//read background, if a file was specified
	if (p_useBack){
		if (p_back_fn != ""){
			delete p_back;
			p_back = new array1D;
			int fail = io->readFromEDF( p_back_fn, p_back );
			MsgLog(name(), info, "histogram of background read\n" << p_back->getHistogramASCII(20) );
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
			delete p_gain;
			p_gain = new array1D;
			int fail = io->readFromEDF( p_gain_fn, p_gain );
			MsgLog(name(), info, "histogram of gain map read\n" << p_gain->getHistogramASCII(20) );
			if (fail){
				MsgLog(name(), warning, "Could not read gainmap, continuing without gain correction!");
				p_useGain = 0;	
			}
		}else{
				MsgLog(name(), warning, "No gain file specified in config file, continuing without gain correction!");
				p_useGain = 0;			
		}
	}
	
	//read mask, if a file was specified
	if (p_useMask){
		if (p_mask_fn != ""){
			delete p_mask;
			p_mask = new array1D;
			int fail = io->readFromEDF( p_mask_fn, p_mask );
			MsgLog(name(), info, "histogram of mask read\n" << p_mask->getHistogramASCII(2) );
			if (fail){
				MsgLog(name(), warning, "Could not read mask, continuing without mask correction!");
				p_useMask = 0;	
			}
		}else{
				MsgLog(name(), warning, "No mask file specified in config file, continuing without mask correction!");
				p_useMask = 0;			
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
	int hist_size = 50;

	shared_ptr<array1D> data = evt.get(IDSTRING_CSPAD_DATA);
	if (data){
		MsgLog(name(), debug, "read event data of size " << data->size() );
		MsgLog(name(), debug, "---histogram of event data before correction---\n" 
			<< data->getHistogramASCII(hist_size) );
		
		//-------------------------------------------------background
		if (p_useBack){
			data.get()->subtractArrayElementwise( p_back );
		}

		//-------------------------------------------------gain		
		if (p_useGain){
			data.get()->divideByArrayElementwise( p_gain );
		}
		
		//-------------------------------------------------mask
		if (p_useMask){
			//data.get()->multiplyByArrayElementwise( p_mask );
			for (unsigned int i = 0; i < data->size(); i++){
				if (p_mask->get(i) < 0.9){
					data->set(i, 0);
				}
			}
		}
		
		MsgLog(name(), debug, "---histogram of event data after correction---\n" 
			<< data->getHistogramASCII(hist_size) );
		
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
