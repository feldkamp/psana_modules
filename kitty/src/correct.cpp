//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class correct...
//
// Author List:
//      Jan Moritz Feldkamp
//		Jonas Alexander Sellberg
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
using ns_cspad_util::create1DFromRawImageCSPAD;

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
	, p_useBack(0)
	, p_back_fn("")
	, p_useGain(0)
	, p_gain_fn("")
	, p_useMask(0)
	, p_mask_fn("")
	, p_usePol(0)
	, p_horzPol(1.)
	, io(0)
	, p_sum(0)
	, p_back(0)
	, p_gain(0)
	, p_mask(0)
	, p_pol(0)
	, p_count(0)
{
  // get the values from configuration or use defaults
	p_useBack			= config   ("useBackground", 	0);
	p_back_fn			= configStr("background", 		"");	
	p_useGain			= config   ("useGainmap", 		0);
	p_gain_fn			= configStr("gainmap", 			"");	
	p_useMask			= config   ("useMask", 			0);
	p_mask_fn			= configStr("mask", 			"");	
	p_usePol			= config   ("usePolarization", 	0);
	p_horzPol			= config   ("horzPolarization", 1.);
	
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
	delete p_pol;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
correct::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "correct::beginJob()" );
	MsgLog(name(), info, "use background correction = '" << p_useBack << "'" );
	MsgLog(name(), info, "background file = '" << p_back_fn << "'" );
	MsgLog(name(), info, "use gain correction = '" << p_useGain << "'" );
	MsgLog(name(), info, "gain file = '" << p_gain_fn << "'" );
	MsgLog(name(), info, "use mask correction = '" << p_useMask << "'" );
	MsgLog(name(), info, "mask file = '" << p_mask_fn << "'" );
	MsgLog(name(), info, "use polarization correction = '" << p_usePol << "'" );
	MsgLog(name(), info, "degree of horizontal polarization = '" << p_horzPol << "'" );
	
	//read background, if a file was specified
	if (p_useBack){
		if (p_back_fn != ""){
			delete p_back;
			p_back = new array1D<double>(nMaxTotalPx);
			
			array2D<double> *img2D = 0;
			int fail = io->readFromFile( p_back_fn, img2D );
 		
			if (!fail){
				MsgLog(name(), info, "background read");
				create1DFromRawImageCSPAD( img2D, p_back );
				MsgLog(name(), info, "histogram\n" << p_back->getHistogramASCII(20) );
			}else{
				MsgLog(name(), warning, "Could not read background, continuing without background subtraction!");
				WAIT;
				p_useBack = 0;	
			}
			delete img2D;
		}else{
			MsgLog(name(), warning, "No background file specified in config file, continuing without background subtraction!");
			p_useBack = 0;			
		}
	}

	//read gainmap, if a file was specified
	if (p_useGain){
		if (p_gain_fn != ""){
			delete p_gain;
			p_gain = new array1D<double>(nMaxTotalPx);

			array2D<double> *img2D = 0;
			int fail = io->readFromFile( p_gain_fn, img2D );

			if (!fail){
				MsgLog(name(), info, "gain map read");
				create1DFromRawImageCSPAD( img2D, p_gain );
				MsgLog(name(), info, "histogram\n" << p_gain->getHistogramASCII(20) );	
			}else{
				MsgLog(name(), warning, "Could not read gainmap, continuing without gain correction!");
				WAIT;
				p_useGain = 0;
			}
			delete img2D;
		}else{
			MsgLog(name(), warning, "No gain file specified in config file, continuing without gain correction!");
			p_useGain = 0;			
		}
	}
	
	//read mask, if a file was specified
	if (p_useMask){
		if (p_mask_fn != ""){
			delete p_mask;
			p_mask = new array1D<double>(nMaxTotalPx);

			array2D<double> *img2D = 0;
			int	fail = io->readFromFile( p_mask_fn, img2D );

			if (!fail){
				MsgLog(name(), info, "mask file read");
				create1DFromRawImageCSPAD( img2D, p_mask );
				MsgLog(name(), info, "histogram\n" << p_mask->getHistogramASCII(2) );	
			}else{
				MsgLog(name(), warning, "Could not read mask, continuing without mask correction!");
				WAIT;
				p_useMask = 0;
			}
			delete img2D;
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
	
	// create polarization correction array
	if (p_usePol) {
		delete p_pol;
		p_pol = new array1D<double>(nMaxTotalPx);
		
		// make sure the degree of horizontal polarization is between 0 and 1 (inclusive)
		if (p_horzPol > 1) {
			p_horzPol = 1;
			MsgLog(name(), warning, "the degree of horizontal polarization was above 100%, lowered to maximum value (1)");
		} else if (p_horzPol < 0) {
			p_horzPol = 0;
			MsgLog(name(), warning, "the degree of horizontal polarization was above 0%, raised to minimum value (0)");
		}
		
		shared_ptr<array1D<double> >	pixPhi_sp = evt.get(IDSTRING_PX_PHI);
		shared_ptr<array1D<double> > pixTwoTheta_sp = evt.get(IDSTRING_PX_TWOTHETA);
		
		if (pixPhi_sp && pixTwoTheta_sp) {
			MsgLog(name(), debug, "read event pixel map of phi of size " << pixPhi_sp->size());
			MsgLog(name(), debug, "read event pixel map of 2theta of size " << pixTwoTheta_sp->size());
			
			// calculate polarization correction for each pixel (from Hura et al JCP 2000)
			for (int i = 0; i < nMaxTotalPx; i++) {
				p_pol->set(i, p_horzPol*(1 - sin(pixPhi_sp->get(i))*sin(pixPhi_sp->get(i))*sin(pixTwoTheta_sp->get(i))*sin(pixTwoTheta_sp->get(i))) + (1 - p_horzPol)*(1 - cos(pixPhi_sp->get(i))*cos(pixPhi_sp->get(i))*sin(pixTwoTheta_sp->get(i))*sin(pixTwoTheta_sp->get(i))));
			}
		} else {
			MsgLog(name(), warning, "could not get pixel maps of phi (address=" << pixPhi_sp.get() << ") and/or 2theta (address=" << pixTwoTheta_sp.get() << ")");
		}
	}
	
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

	shared_ptr<array1D<double> > data_sp = evt.get(IDSTRING_CSPAD_DATA);
	
	if (data_sp){
		MsgLog(name(), debug, "read event data of size " << data_sp->size() );
		MsgLog(name(), debug, "\n---histogram of event data before correction---\n" 
			<< data_sp->getHistogramASCII(hist_size) );
		
		int fail = 0;
		//-------------------------------------------------background
		if (p_useBack){
			fail += data_sp->subtractArrayElementwise( p_back );
		}

		//-------------------------------------------------gain		
		if (p_useGain){
			fail += data_sp->divideByArrayElementwise( p_gain );
		}
		
		//-------------------------------------------------polarization
		if (p_usePol){
			fail += data_sp->divideByArrayElementwise( p_pol );
		}
		
		//-------------------------------------------------mask
		if (p_useMask){
			fail += data_sp->applyMask( p_mask );
		}
		
		if (fail){
			MsgLog(name(), error, "At least one correction operation failed.");
			WAIT;
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
