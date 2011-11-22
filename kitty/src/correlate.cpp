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
#include "kitty/util.h"
using ns_cspad_util::create1DFromRawImageCSPAD;

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
	, p_pix1_sp()
	, p_pix2_sp()
	, p_mask(0)
	, p_LUT(0)
	, p_polarAvg_sp()
	, p_corrAvg_sp()
	, p_qAvg_sp()
	, p_iAvg_sp()
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
	array2D *img2D = 0;
	int fail = io->readFromFile( p_mask_fn, img2D );
	if (!fail){
		create1DFromRawImageCSPAD( img2D, p_mask );
		MsgLog(name(), info, "correlation mask loaded successfully" );
	}else{
		MsgLog(name(), info, "correlation mask NOT loaded" );		
		WAIT;
	}
	delete img2D;

}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
correlate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::beginRun()" );
	
	p_outputPrefix = *( (shared_ptr<std::string>) evt.get(IDSTRING_OUTPUT_PREFIX) ).get();

	p_pix1_sp = evt.get(IDSTRING_PX_X_int); 
	p_pix2_sp = evt.get(IDSTRING_PX_Y_int); 
	
	if (p_pix1_sp && p_pix2_sp){
		MsgLog(name(), info, "read data for pix1(n=" << p_pix1_sp->size() << "), pix2(n=" << p_pix2_sp->size() << ")" );
	}else{
		MsgLog(name(), warning, "could not get data from pixX(addr=" << p_pix1_sp.get() << ") or pixY(addr=" << p_pix2_sp.get() << ")" );
	}

	//before everything starts, create a dummy CrossCorrelator to set up some things
	CrossCorrelator *dummy_cc = new CrossCorrelator(p_pix1_sp.get(), p_pix1_sp.get(), p_pix2_sp.get(), p_nPhi, p_nQ1);
	p_nLag = dummy_cc->nLag();
	if ( p_alg == 2 || p_alg == 4 ){
		//prepare lookup table once here, so it doesn't have to be done every time
		MsgLog(name(), info, "creating lookup table");
		dummy_cc->createLookupTable(p_LUTy, p_LUTx);
		delete p_LUT;
		p_LUT = new array2D( *(dummy_cc->lookupTable()) );
		//io->writeToEDF( "LUT.edf", p_LUT ); //write to disk for debugging
	}
	delete dummy_cc;
	
	//resize average-keeping arrays after p_nLag was set by the dummy cross-correlator
	p_polarAvg_sp = shared_ptr<array2D>( new array2D(p_nQ1,p_nPhi) );
	p_corrAvg_sp = shared_ptr<array2D>( new array2D(p_nQ1,p_nLag) );
	p_qAvg_sp = shared_ptr<array1D>( new array1D(p_nQ1) );
	p_iAvg_sp = shared_ptr<array1D>( new array1D(p_nQ1) );
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

	shared_ptr<array1D> data_sp = evt.get(IDSTRING_CSPAD_DATA);
	string eventname_str = *( (shared_ptr<std::string>) evt.get(IDSTRING_CUSTOM_EVENTNAME) ).get();
	
	if (data_sp){
		MsgLog(name(), debug, "read event data of size " << data_sp->size() << " from ID string " << IDSTRING_CSPAD_DATA);
		
		//create cross correlator object that takes care of the computations
		//the arguments that are passed to the constructor determine if calculations are 2D/3D and with/without mask
		CrossCorrelator *cc = 0;
		
		if (p_autoCorrelateOnly) {
			if (p_useMask) {											//auto-correlation 2D case, with mask
				cc = new CrossCorrelator( data_sp.get(), p_pix1_sp.get(), p_pix2_sp.get(), p_nPhi, p_nQ1, 0, p_mask );
			} else { 													//auto-correlation 2D case, no mask
				cc = new CrossCorrelator( data_sp.get(), p_pix1_sp.get(), p_pix2_sp.get(), p_nPhi, p_nQ1 );
			}
		} else {
			if (p_useMask){												//full cross-correlation 3D case, with mask
				cc = new CrossCorrelator( data_sp.get(), p_pix1_sp.get(), p_pix2_sp.get(), p_nPhi, p_nQ1, p_nQ2, p_mask );
			} else { 													//full cross-correlation 3D case, no mask
				cc = new CrossCorrelator( data_sp.get(), p_pix1_sp.get(), p_pix2_sp.get(), p_nPhi, p_nQ1, p_nQ2);
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
		
		MsgLog(name(), debug, "updating running sums.");
		p_polarAvg_sp->addArrayElementwise( cc->polar() );
		p_corrAvg_sp->addArrayElementwise( cc->autoCorr() );
		p_qAvg_sp->addArrayElementwise( cc->qAvg() );
		p_iAvg_sp->addArrayElementwise( cc->iAvg() );
		
		MsgLog(name(), debug, "cc->polar dims: rows,cols=(" << cc->polar()->dim1() << ", " << cc->polar()->dim2() << ")"  );
		MsgLog(name(), debug, "p_polarAvg dims: rows,cols=(" << p_polarAvg_sp->dim1() << ", " << p_polarAvg_sp->dim2() << ")"  );
		MsgLog(name(), debug, "cc->autoCorr dims: rows,cols=(" << cc->autoCorr()->dim1() << ", " << cc->autoCorr()->dim2() << ")"  );
		MsgLog(name(), debug, "p_corrAvg dims: rows,cols=(" << p_corrAvg_sp->dim1() << ", " << p_corrAvg_sp->dim2() << ")"  );
		
		if ( p_singleOutput ){
			if ( !(p_count % p_singleOutput) ){
				MsgLog(name(), debug, "writing (single event) files.");
				if (p_h5Out){
					string ext = ".h5";
					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_xaca"+ext, cc->autoCorr() );
					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_polar"+ext, cc->polar() );
					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_q"+ext, cc->qAvg() );
					io->writeToHDF5( p_outputPrefix+"_evt"+eventname_str+"_i"+ext, cc->iAvg() );			
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
	p_polarAvg_sp->divideByValue( p_count );
	p_corrAvg_sp->divideByValue( p_count );
	p_qAvg_sp->divideByValue( p_count );
	p_iAvg_sp->divideByValue( p_count );
	
	//convert SAXS output to 2D, so they can be written to disk as images (with y-dim == 1)
	shared_ptr<array2D> qAvg2D_sp = shared_ptr<array2D>( new array2D( p_qAvg_sp.get(), p_qAvg_sp->dim1(), 1 ) );
	shared_ptr<array2D> iAvg2D_sp = shared_ptr<array2D>( new array2D( p_iAvg_sp.get(), p_iAvg_sp->dim1(), 1 ) );
	
	//HDF5 output
	if (p_h5Out){
		string ext = ".h5";
		if (p_autoCorrelateOnly){
			io->writeToHDF5( p_outputPrefix+"_avg_polar"+ext, p_polarAvg_sp.get());
			io->writeToHDF5( p_outputPrefix+"_avg_xaca"+ext, p_corrAvg_sp.get() );
		}else{
			MsgLog(name(), warning, "WARNING. No HDF5 output for 3D cross-correlation case implemented, yet!" );
		}
		io->writeToHDF5( p_outputPrefix+"_avg_qAvg"+ext, qAvg2D_sp.get() );
		io->writeToHDF5( p_outputPrefix+"_avg_iAvg"+ext, iAvg2D_sp.get() );
	}
	//EDF output
	if (p_edfOut){
		string ext = ".edf";
		if (p_autoCorrelateOnly){
			io->writeToEDF( p_outputPrefix+"_avg_polar"+ext, p_polarAvg_sp.get() );
			io->writeToEDF( p_outputPrefix+"_avg_xaca"+ext, p_corrAvg_sp.get() );
		}else{
			MsgLog(name(), warning, "WARNING. No EDF output for 3D cross-correlation case implemented, yet!" );
		}
		io->writeToEDF( p_outputPrefix+"_avg_qAvg"+ext, qAvg2D_sp.get() );
		io->writeToEDF( p_outputPrefix+"_avg_iAvg"+ext, iAvg2D_sp.get() );
	}
	//TIFF image output
	if (p_tifOut){
		string ext = ".tif";
		if (p_autoCorrelateOnly){
			io->writeToTiff( p_outputPrefix+"_avg_polar"+ext, p_polarAvg_sp.get(), 1 );			// 0: unscaled, 1: scaled
			io->writeToTiff( p_outputPrefix+"_avg_xaca"+ext, p_corrAvg_sp.get(), 1 );			// 0: unscaled, 1: scaled
		}else{
			MsgLog(name(), warning, "WARNING. No tiff output for 3D cross-correlation case implemented, yet!" );
			//one possibility would be to write a stack of tiffs, one for each of the outer q values
		}
		
		io->writeToTiff( p_outputPrefix+"_avg_qAvg"+ext, qAvg2D_sp.get(), 1 );
		io->writeToTiff( p_outputPrefix+"_avg_iAvg"+ext, iAvg2D_sp.get(), 1 );
	}
	
}

} // namespace kitty
