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
#include <iomanip>

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
	, p_tifOut(0)
	, p_edfOut(0)
	, p_h5Out(0)
	, p_singleOutput(0)
	, p_outputPrefix("")
	, p_mask_fn("")
	, p_useMask(0)
	, p_nPhi(0)
	, p_nQ1(0)
	, p_nQ2(0)
	, p_nLag(0)
	, p_alg(0)
	, p_autoCorrelateOnly(0)
	, p_startQ(0)
	, p_stopQ(0)
	, p_LUTx(0)
	, p_LUTy(0)
	, io(0)
	, p_pixX_sp()
	, p_pixY_sp()
	, p_mask(0)
	, p_LUT(0)
	, p_polarAvg(0)
	, p_corrAvg(0)
	, p_qAvg(0)
	, p_iAvg(0)
	, p_count(0)
{
	p_tifOut 			= config   ("tifOut", 				0);
	p_edfOut 			= config   ("edfOut", 				1);
	p_h5Out 			= config   ("h5Out", 				0);
	p_singleOutput		= config   ("singleOutput",			0);
	
	p_mask_fn			= configStr("mask", 				"");	
	p_useMask			= config   ("useMask", 				0);
		
	p_nPhi 				= config   ("nPhi",					128);
	p_nQ1 				= config   ("nQ1",					1);
	p_nQ2 				= config   ("nQ2",					1);
	
	p_alg				= config   ("algorithm",			1);
	p_autoCorrelateOnly = config   ("autoCorrelateOnly",	1);	
	
	p_startQ			= config   ("startQ",				0);
	p_stopQ				= config   ("stopQ",				p_startQ+p_nQ1);	

	p_LUTx				= config   ("LUTx",					1000);
	p_LUTy				= config   ("LUTy",					1000);
	
	io = new arraydataIO();

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
	delete p_qAvg;
	delete p_iAvg;
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
	MsgLog(name(), info, "mask file name    = '" << p_mask_fn << "'" );
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
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
correlate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::beginRun()" );
	
	p_outputPrefix = *( (shared_ptr<std::string>) evt.get(IDSTRING_OUTPUT_PREFIX) ).get();

	p_pixX_sp = evt.get(IDSTRING_PX_X_int); 
	p_pixY_sp = evt.get(IDSTRING_PX_Y_int); 
	
	if (p_pixX_sp && p_pixY_sp){
		MsgLog(name(), info, "read data for pixX(n=" << p_pixX_sp->size() << "), pixY(n=" << p_pixY_sp->size() << ")" );
	}else{
		MsgLog(name(), warning, "could not get data from pixX(addr=" << p_pixX_sp.get() << ") or pixY(addr=" << p_pixY_sp.get() << ")" );
	}

	//before everything starts, create a dummy CrossCorrelator to set up some things
	CrossCorrelator *dummy_cc = new CrossCorrelator(p_pixY_sp.get(), p_pixY_sp.get(), p_pixX_sp.get(), p_nPhi, p_nQ1);
	p_nLag = dummy_cc->nLag();
	if ( p_alg == 2 || p_alg == 4 ){
		//prepare lookup table once here, so it doesn't have to be done every time
		dummy_cc->createLookupTable(p_LUTy, p_LUTx);
		delete p_LUT;
		p_LUT = new array2D( *(dummy_cc->lookupTable()) );
		//io->writeToEDF( "LUT.edf", p_LUT ); //write to disk for debugging
	}
	delete dummy_cc;
	
	//resize average-keeping arrays after p_nLag was set by the dummy cross-correlator
	delete p_polarAvg;
	p_polarAvg = new array2D( p_nQ1, p_nPhi );
	delete p_corrAvg;
	p_corrAvg = new array2D( p_nQ1, p_nLag );
	delete p_qAvg;
	p_qAvg = new array1D( p_nQ1 );
	delete p_iAvg;
	p_iAvg = new array1D( p_nQ1 );
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

	array1D *data = ( (shared_ptr<array1D>) evt.get(IDSTRING_CSPAD_DATA) ).get();
	string eventname_str = *( (shared_ptr<std::string>) evt.get(IDSTRING_CUSTOM_EVENTNAME) ).get();
	
	if (data){
		MsgLog(name(), debug, "read event data of size " << data->size() << " from ID string " << IDSTRING_CSPAD_DATA);
		
		//create cross correlator object that takes care of the computations
		//the arguments that are passed to the constructor determine 2D/3D calculations with/without mask
		CrossCorrelator *cc = 0;
		
		//-----------------------------ATTENTION----------------------------------		
		// THE X & Y ARRAYS GENERATED FROM THE CALIBRATION DATA
		// SEEM TO HAVE SWAPPED MEANING THAN IN CrossCorrelator
		// SWITCHING THEM AROUND IN THE CONSTRUCTOR SEEM TO MAKE THINGS RIGHT
		//------------------------------------------------------------------------
				
		if (p_autoCorrelateOnly) {
			if (p_useMask) {											//auto-correlation 2D case, with mask
				cc = new CrossCorrelator( data, p_pixY_sp.get(), p_pixX_sp.get(), p_nPhi, p_nQ1, 0, p_mask );
			} else { 															//auto-correlation 2D case, no mask
				cc = new CrossCorrelator( data, p_pixY_sp.get(), p_pixX_sp.get(), p_nPhi, p_nQ1 );
			}
		} else {
			if (p_useMask){												//full cross-correlation 3D case, with mask
				cc = new CrossCorrelator( data, p_pixY_sp.get(), p_pixX_sp.get(), p_nPhi, p_nQ1, p_nQ2, p_mask );
			} else { 															//full cross-correlation 3D case, no mask
				cc = new CrossCorrelator( data, p_pixY_sp.get(), p_pixX_sp.get(), p_nQ1, p_nQ1, p_nQ2);
			}
		}
		
		//check psana's debug level, set a debug flag for cc, if it's at least 'info'...
		MsgLogger::MsgLogLevel lvl(MsgLogger::MsgLogLevel::info);
		if ( MsgLogger::MsgLogger().logging(lvl) ) {
			cc->setDebug(1);
		}
		
		if (p_LUT){
			cc->setLookupTable( p_LUT );
		}else{
			MsgLog(name(), debug, "no lookup table set (since it's not needed in this algorithm)");
		}
		
		MsgLog(name(), info, "calling alg " << p_alg << ", startQ=" << p_startQ << ", stopQ=" << p_stopQ );
		
		cc->run(p_startQ, p_stopQ, p_alg, true);

		MsgLog(name(), info, "correlation calculation done. writing (single event) files.");
		if ( p_singleOutput ){
			if ( !(p_count % p_singleOutput) ){
				
				if (p_h5Out){
					string ext = ".h5";
					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_xaca"+ext, cc->autoCorr() );
					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_polar"+ext, cc->polar() );
					//1D output not implemented just yet, using edf for the moment
//					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_q"+ext, cc->qAvg() );
//					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_i"+ext, cc->iAvg() );
					ext = ".edf";
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_q"+ext, cc->qAvg() );
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_i"+ext, cc->iAvg() );				
				}
				if (p_edfOut){
					string ext = ".edf";
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_xaca"+ext, cc->autoCorr() );
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_polar"+ext, cc->polar() );
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_q"+ext, cc->qAvg() );
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_i"+ext, cc->iAvg() );
				}
				if (p_tifOut){
					string ext = ".tif";
					io->writeToTiff( p_outputPrefix+"_evt"+eventname_str+"_xaca"+ext, cc->autoCorr() );
					io->writeToTiff( p_outputPrefix+"_evt"+eventname_str+"_polar"+ext, cc->polar() );
					//1D output not implemented just yet, using edf for the moment
//					io->writeToTiff( p_outputPrefix+"_evt"+eventname_str+"_q"+ext, cc->qAvg() );
//					io->writeToTiff( p_outputPrefix+"_evt"+eventname_str+"_i"+ext, cc->iAvg() );
					ext = ".edf";
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_q"+ext, cc->qAvg() );
					io->writeToEDF( p_outputPrefix+"_evt"+eventname_str+"_i"+ext, cc->iAvg() );				
				}
			}//if no modulo
		}//if singleout
		
		MsgLog(name(), info, "updating running sums.");
		p_polarAvg->addArrayElementwise( cc->polar() );
		p_corrAvg->addArrayElementwise( cc->autoCorr() );
		p_qAvg->addArrayElementwise( cc->qAvg() );
		p_iAvg->addArrayElementwise( cc->iAvg() );
		
		MsgLog(name(), debug, "cc->polar dims: rows,cols=(" << cc->polar()->dim1() << ", " << cc->polar()->dim2() << ")"  );
		MsgLog(name(), debug, "p_polarAvg dims: rows,cols=(" << p_polarAvg->dim1() << ", " << p_polarAvg->dim2() << ")"  );
		MsgLog(name(), debug, "cc->autoCorr dims: rows,cols=(" << cc->autoCorr()->dim1() << ", " << cc->autoCorr()->dim2() << ")"  );
		MsgLog(name(), debug, "p_corrAvg dims: rows,cols=(" << p_corrAvg->dim1() << ", " << p_corrAvg->dim2() << ")"  );
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
	p_qAvg->divideByValue( p_count );
	p_iAvg->divideByValue( p_count );
	
	//convert SAXS output to 2D, so they can be written to disk as images (with y-dim == 1)
	array2D* qAvg2D = new array2D( p_qAvg, p_qAvg->dim1(), 1 );
	array2D* iAvg2D = new array2D( p_iAvg, p_iAvg->dim1(), 1 );
	
	//HDF5 output
	if (p_h5Out){
		string ext = ".h5";
		if (p_autoCorrelateOnly){
			io->writeToHDF5( p_outputPrefix+"_avg_polar"+ext, p_polarAvg);
			io->writeToHDF5( p_outputPrefix+"_avg_xaca"+ext, p_corrAvg );
		}else{
			MsgLog(name(), warning, "WARNING. No HDF5 output for 3D cross-correlation case implemented, yet!" );
		}
		io->writeToHDF5( p_outputPrefix+"_avg_qAvg"+ext, qAvg2D );
		io->writeToHDF5( p_outputPrefix+"_avg_iAvg"+ext, iAvg2D );
	}
	//EDF output
	if (p_edfOut){
		string ext = ".edf";
		if (p_autoCorrelateOnly){
			io->writeToEDF( p_outputPrefix+"_avg_polar"+ext, p_polarAvg );
			io->writeToEDF( p_outputPrefix+"_avg_xaca"+ext, p_corrAvg );
		}else{
			MsgLog(name(), warning, "WARNING. No EDF output for 3D cross-correlation case implemented, yet!" );
		}
		io->writeToEDF( p_outputPrefix+"_avg_qAvg"+ext, qAvg2D );
		io->writeToEDF( p_outputPrefix+"_avg_iAvg"+ext, iAvg2D );
	}
	//TIFF image output
	if (p_tifOut){
		string ext = ".tif";
		if (p_autoCorrelateOnly){
			io->writeToTiff( p_outputPrefix+"_avg_polar"+ext, p_polarAvg, 1 );			// 0: unscaled, 1: scaled
			io->writeToTiff( p_outputPrefix+"_avg_xaca"+ext, p_corrAvg, 1 );			// 0: unscaled, 1: scaled
		}else{
			MsgLog(name(), warning, "WARNING. No tiff output for 3D cross-correlation case implemented, yet!" );
			//one possibility would be to write a stack of tiffs, one for each of the outer q values
		}
		
		io->writeToTiff( p_outputPrefix+"_avg_qAvg"+ext, qAvg2D, 1 );
		io->writeToTiff( p_outputPrefix+"_avg_iAvg"+ext, iAvg2D, 1 );
	}
	
	delete qAvg2D;
	delete iAvg2D;
}

} // namespace kitty
