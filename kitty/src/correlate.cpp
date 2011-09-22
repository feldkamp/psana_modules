//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class correlate...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/correlate.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "PSEvt/EventId.h"
#include "PSCalib/CSPadCalibPars.h"

#include "kitty/constants.h"
#include "kitty/crosscorrelator.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

// This declares this class as psana module
using namespace kitty;
PSANA_MODULE_FACTORY(correlate)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
correlate::correlate (const std::string& name)
  : Module(name)
{
	p_tifOut 			= config   ("tifOut", 				0);
	p_edfOut 			= config   ("edfOut", 				1);
	p_h5Out 			= config   ("h5Out", 				0);
	p_autoCorrelateOnly = config   ("autoCorrelateOnly",	1);
	p_mask_fn			= configStr("mask", 				"");	
	p_useMask			= config   ("useMask", 				0);
	p_singleOutput		= config   ("singleOutput",			0);
	p_outputPrefix		= configStr("outputPrefix", 		"corr");
	
	p_nPhi 				= config   ("nPhi",					128);
	p_nQ1 				= config   ("nQ1",					1);
	p_nQ2 				= config   ("nQ2",					1);
	
	p_alg				= config   ("algorithm",			1);
	
	p_startQ			= config   ("startQ",				0);
	p_stopQ				= config   ("stopQ",				p_startQ+p_nQ1);	

	p_LUTx				= config   ("LUTx",					100);
	p_LUTy				= config   ("LUTy",					100);

	p_nLag = 0;
	
	io = new arraydataIO;
	
	//initialize arrays to something
	p_mask = new array1D;
	p_LUT = new array2D;
	p_polarAvg = new array2D;
	p_corrAvg = new array2D;
	
	p_count = 0;
}

//--------------
// Destructor --
//--------------
correlate::~correlate ()
{
	delete io;
	
	delete p_mask;
	delete p_LUT;
	delete p_polarAvg;
	delete p_corrAvg;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
correlate::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "correlate::beginJob()" );
	MsgLog(name(), info, "tifOut            = '" << p_tifOut << "'" );
	MsgLog(name(), info, "edfOut            = '" << p_edfOut << "'" );
	MsgLog(name(), info, "h5Out             = '" << p_h5Out << "'" );
	MsgLog(name(), info, "autoCorrelateOnly = '" << p_autoCorrelateOnly << "'" );
	MsgLog(name(), info, "useMask           = '" << p_useMask << "'" );
	MsgLog(name(), info, "mask              = '" << p_mask << "'" );
	MsgLog(name(), info, "singleOutput      = '" << p_singleOutput << "'" );
	MsgLog(name(), info, "nPhi              = '" << p_nPhi << "'" );
	MsgLog(name(), info, "nQ1               = '" << p_nQ1 << "'" );
	MsgLog(name(), info, "nQ2               = '" << p_nQ2 << "'" );
	MsgLog(name(), info, "algorithm         = '" << p_alg << "'" );
	MsgLog(name(), info, "startQ            = '" << p_startQ << "'" );
	MsgLog(name(), info, "stopQ             = '" << p_stopQ << "'" );
	MsgLog(name(), info, "LUTx              = '" << p_LUTx << "'" );
	MsgLog(name(), info, "LUTy              = '" << p_LUTy << "'" );


	//load bad pixel mask
	io->readFromEDF( p_mask_fn, p_mask );
	
	p_pixX_sp = evt.get(IDSTRING_PX_X_int);
	p_pixY_sp = evt.get(IDSTRING_PX_Y_int);
	if (p_pixX_sp && p_pixY_sp){
		MsgLog(name(), info, "read data for pixX(n=" << p_pixX_sp->size() << "), pixY(n=" << p_pixY_sp->size() << ")" );
	}else{
		MsgLog(name(), warning, "could not get data from pixX(addr=" << p_pixX_sp << ") or pixY(addr=" << p_pixY_sp << ")" );
	}

	//before everything starts, create a dummy CrossCorrelator to set up some things
	CrossCorrelator *dummy_cc = new CrossCorrelator(p_pixX_sp.get(), p_pixX_sp.get(), p_pixY_sp.get(), p_nPhi, p_nQ1);
	p_nLag = dummy_cc->nLag();
	if ( p_alg == 2 || p_alg == 4 ){
		//prepare lookup table once here, so it doesn't have to be done every time
		dummy_cc->createLookupTable(p_LUTy, p_LUTx);
		delete p_LUT;
		p_LUT = new array2D( *(dummy_cc->lookupTable()) );
		io->writeToEDF( "LUT.edf", p_LUT );
	}
	delete dummy_cc;
	
	//resize average-keeping arrays after p_nLag was set by the dummy cross-correlator
	delete p_polarAvg;
	p_polarAvg = new array2D( p_nQ1, p_nPhi );
	delete p_corrAvg;
	p_corrAvg = new array2D( p_nQ1, p_nLag );
	
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
correlate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::beginRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
correlate::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
correlate::event(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::event()" );

	shared_ptr<array1D> data = evt.get(IDSTRING_CSPAD_DATA);
	shared_ptr<PSEvt::EventId> eventId = evt.get();
	
	if (data){
		MsgLog(name(), info, "read event data of size " << data->size() );

		std::ostringstream osst;
		osst << "t" << eventId->time();
		string eventname_str = osst.str();
		
		//create cross correlator object that takes care of the computations
		//the arguments that are passed to the constructor determine 2D/3D calculations with/without mask
		CrossCorrelator *cc = NULL;
		if (p_autoCorrelateOnly) {
			if (p_useMask) {											//auto-correlation 2D case, with mask
				cc = new CrossCorrelator( data.get(), p_pixX_sp.get(), p_pixY_sp.get(), p_nPhi, p_nQ1, 0, p_mask );
			} else { 															//auto-correlation 2D case, no mask
				cc = new CrossCorrelator( data.get(), p_pixX_sp.get(), p_pixY_sp.get(), p_nPhi, p_nQ1 );
			}
		} else {
			if (p_useMask){												//full cross-correlation 3D case, with mask
				cc = new CrossCorrelator( data.get(), p_pixX_sp.get(), p_pixY_sp.get(), p_nPhi, p_nQ1, p_nQ2, p_mask );
			} else { 															//full cross-correlation 3D case, no mask
				cc = new CrossCorrelator( data.get(), p_pixX_sp.get(), p_pixY_sp.get(), p_nQ1, p_nQ1, p_nQ2);
			}
		}
		
		//don't know how to read psana's debug level... that could be passed here
		cc->setDebug(1); 

		MsgLog(name(), info, "calling alg " << p_alg << ", startQ=" << p_startQ << ", stopQ=" << p_stopQ );
		
		switch (p_alg) {
			case 1:
				MsgLog(name(), info, "DIRECT COORDINATES, DIRECT XCCA (algorithm 1)");
				cc->calculatePolarCoordinates( p_startQ, p_stopQ );
				cc->calculateSAXS( p_startQ, p_stopQ );
				cc->calculateXCCA( p_startQ, p_stopQ );	
			break;
			case 2:
				MsgLog(name(), info, "FAST COORDINATES, FAST XCCA (algorithm 2)");
				cc->setLookupTable( p_LUT );
				cc->calculatePolarCoordinates_FAST( p_startQ, p_stopQ );
				cc->calculateXCCA_FAST();
				break;
			case 3:
				MsgLog(name(), info, "DIRECT COORDINATES, FAST XCCA (algorithm 3)");
				cc->setLookupTable( p_LUT );
				cc->calculatePolarCoordinates( p_startQ, p_stopQ );
				cc->calculateXCCA_FAST();
				break;
			case 4:
				MsgLog(name(), info, "FAST COORDINATES, DIRECT XCCA (algorithm 4)");
				cc->setLookupTable( p_LUT );
				cc->calculatePolarCoordinates_FAST( p_startQ, p_stopQ );
				cc->calculateXCCA( p_startQ, p_stopQ );
				break;
			default:
				MsgLog(name(), error,  "Choice of algorithm is invalid. Aborting.");
				return; 
		}//switch

		MsgLog(name(), info, "correlation calculation done. writing (single event) files.");
		if ( p_singleOutput ){
			io->writeToEDF( "evt"+eventname_str+"_xaca.edf", cc->autoCorr() );
			io->writeToEDF( "evt"+eventname_str+"_polar.edf", cc->polar() );
		}
		
		MsgLog(name(), info, "updating running sums.");
		p_polarAvg->addArrayElementwise( cc->polar() );
		p_corrAvg->addArrayElementwise( cc->autoCorr() );
		
		delete cc;
		
		p_count++;
	}else{
		MsgLog(name(), warning, "could not get CSPAD data" );
	}

}
  
  
/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
correlate::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
correlate::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
correlate::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::endJob()" );
	p_polarAvg->divideByValue( p_count );
	p_corrAvg->divideByValue( p_count );
	
	//TIFF image output
	if (p_tifOut){
		if (p_autoCorrelateOnly){
			io->writeToTiff( p_outputPrefix+"_polar.tif", p_polarAvg, 1 );			// 0: unscaled, 1: scaled
			io->writeToTiff( p_outputPrefix+"_xaca.tif", p_corrAvg, 1 );			// 0: unscaled, 1: scaled
		}else{
			MsgLog(name(), warning, "WARNING. No tiff output for 3D cross-correlation case implemented, yet!" );
			//one possibility would be to write a stack of tiffs, one for each of the outer q values
		}
	}
	//EDF output
	if (p_edfOut){
		if (p_autoCorrelateOnly){
			io->writeToEDF( p_outputPrefix+"_polar.edf", p_polarAvg );
			io->writeToEDF( p_outputPrefix+"_xaca.edf", p_corrAvg );
		}else{
			MsgLog(name(), warning, "WARNING. No EDF output for 3D cross-correlation case implemented, yet!" );
		}
	}
	//HDF5 output
	if (p_h5Out){
		if (p_autoCorrelateOnly){
			io->writeToHDF5( p_outputPrefix+"_polar.h5", p_polarAvg);
			io->writeToHDF5( p_outputPrefix+"_xaca.h5", p_corrAvg );
		}else{
			MsgLog(name(), warning, "WARNING. No HDF5 output for 3D cross-correlation case implemented, yet!" );
		}
	}
}

} // namespace kitty
