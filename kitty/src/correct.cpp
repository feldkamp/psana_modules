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
PSANA_MODULE_FACTORY(correct)

//#define WAIT usleep(1000000)
#define WAIT std::cerr << "continue by pressing the <return> key..."; std::cin.get();

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
correct::correct (const std::string& name)
	: Module(name)
	, p_back_fn("")
	, p_useBack(0)
	, p_gain_fn("")
	, p_useGain(0)
	, p_mask_fn("")
	, p_useMask(0)
	, io(0)
	, p_sum(0)
	, p_back(0)
	, p_gain(0)
	, p_mask(0)
	, p_count(0)
{
  // get the values from configuration or use defaults
	p_back_fn			= configStr("background", 		"");	
	p_useBack			= config   ("useBackground", 	0);
	p_gain_fn			= configStr("gainmap", 			"");	
	p_useGain			= config   ("useGainmap", 		0);
	p_mask_fn			= configStr("mask", 			"");	
	p_useMask			= config   ("useMask", 			0);
	
	io = new arraydataIO();
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
			if (!p_back) 
				p_back = new array1D(nMaxTotalPx);
			int fail = 0;
			string ext = getExt( p_back_fn );
			if (ext == "edf"){											// read 1D
				fail = io->readFromEDF( p_back_fn, p_back );
			}else if (ext == "h5" || ext == "hdf5" || ext == "H5"){		// read 2D 'raw' image
				array2D *from_file = new array2D;
				fail = io->readFromHDF5( p_back_fn, from_file );
				if (!fail) create1DFromRawImageCSPAD(from_file, p_back);
				delete from_file;
			}else{
				MsgLog(name(), error, "Extension '" << ext << "' found in '" << p_back_fn << "' is not valid. "
					<< "Should be edf or h5 ");
			}
			if (!fail){
				MsgLog(name(), info, "histogram of background read\n" << p_back->getHistogramASCII(20) );
			}else{
				MsgLog(name(), warning, "Could not read background, continuing without background subtraction!");
				WAIT;
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
			if (!p_gain) 
				p_gain = new array1D(nMaxTotalPx);
			int fail = 0;
			string ext = getExt( p_gain_fn );
			if (ext == "edf"){									// read 1D
				fail = io->readFromEDF( p_gain_fn, p_gain );
			}else if (ext == "h5" || ext == "hdf5" || ext == "H5"){		// read 2D 'raw' image
				array2D *from_file = new array2D;
				fail = io->readFromHDF5( p_gain_fn, from_file );
				if (!fail) {
					MsgLog(name(), info, "rows: " << from_file->dim1() << ", cols: " << from_file->dim2() );
					create1DFromRawImageCSPAD(from_file, p_gain);
				}
				delete from_file;
			}else{
				MsgLog(name(), error, "Extension '" << ext << "' found in '" << p_gain_fn << "' is not valid. "
					<< "Should be edf or h5 ");
			}
			if (!fail){
				MsgLog(name(), info, "histogram of gain map read\n" << p_gain->getHistogramASCII(20) );	
			}else{
				MsgLog(name(), warning, "Could not read gainmap, continuing without gain correction!");
				p_useGain = 0;
				WAIT;
			}
		}else{
			MsgLog(name(), warning, "No gain file specified in config file, continuing without gain correction!");
			p_useGain = 0;			
		}
	}
	
	//read mask, if a file was specified
	if (p_useMask){
		if (p_mask_fn != ""){
			if (!p_mask)
				p_mask = new array1D(nMaxTotalPx);
			int fail = 0;
			string ext = getExt( p_mask_fn );
			if (ext == "edf"){									// read 1D
				fail = io->readFromEDF( p_mask_fn, p_mask );
			}else if (ext == "h5" || ext == "hdf5" || ext == "H5"){		// read 2D 'raw' image
				array2D *from_file = new array2D;
				fail = io->readFromHDF5( p_mask_fn, from_file );
				if (!fail) create1DFromRawImageCSPAD(from_file, p_mask);
				delete from_file;
			}else{
				MsgLog(name(), error, "Extension '" << ext << "' found in '" << p_mask_fn << "' is not valid. "
					<< "Should be edf or h5 ");
			}
			if (!fail){
				MsgLog(name(), info, "histogram of mask read\n" << p_mask->getHistogramASCII(2) );	
			}else{
				MsgLog(name(), warning, "Could not read mask, continuing without mask correction!");
				p_useMask = 0;	
				WAIT;
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

	shared_ptr<array1D> data_sp = evt.get(IDSTRING_CSPAD_DATA);
	
	
	if (data_sp){
		MsgLog(name(), debug, "read event data of size " << data_sp->size() );
		MsgLog(name(), debug, "\n---histogram of event data before correction---\n" 
			<< data_sp->getHistogramASCII(hist_size) );
		
		//-------------------------------------------------background
		if (p_useBack){
			data_sp->subtractArrayElementwise( p_back );
		}

		//-------------------------------------------------gain		
		if (p_useGain){
			data_sp->divideByArrayElementwise( p_gain );
		}
		
		//-------------------------------------------------mask
		if (p_useMask){
			for (unsigned int i = 0; i < data_sp->size(); i++){
				if (p_mask->get(i) < 0.9){
					data_sp->set(i, 0);
				}
			}
		}
		
		MsgLog(name(), debug, "\n---histogram of event data after correction---\n" 
			<< data_sp->getHistogramASCII(hist_size) );
		
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
