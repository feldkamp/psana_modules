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
  , m_src()
{
	// get the values from configuration or use defaults
	m_src				= configStr("source",				"CxiDs1.0:Cspad.0");
	m_calibDir      	= configStr("calibDir",				"/reg/d/psdm/CXI/cxi35711/calib");
	m_typeGroupName 	= configStr("typeGroupName",		"CsPad::CalibV1");
	m_tiltIsApplied 	= config   ("tiltIsApplied",		true);								//tilt angle correction
	m_runNumber			= config   ("runNumber",     		0);
	
	p_tifOut 			= config   ("tifOut", 				0);
	p_edfOut 			= config   ("edfOut", 				1);
	p_h5Out 			= config   ("h5Out", 				0);
	p_autoCorrelateOnly = config   ("autoCorrelateOnly",	1);
	p_useBadPixelMask 	= config   ("useBadPixelMask",		1);
	p_singleOutput		= config   ("singleOutput",			0);

	p_nPhi 				= config   ("nPhi",					128);
	p_nQ1 				= config   ("nQ1",					1);
	p_nQ2 				= config   ("nQ2",					1);
	
	p_alg				= config   ("algorithm",			1);
	
	p_startQ			= config   ("startQ",				0);
	p_stopQ				= config   ("stopQ",				p_nQ1);	

	p_LUTx				= config   ("LUTx",					100);
	p_LUTy				= config   ("LUTy",					100);

	io = new arraydataIO;
	p_pixX = NULL;
	p_pixY = NULL;
	p_badPixelMask = NULL;
	p_LUT = NULL;
	
	p_polarAvg = new array2D( p_nQ1, p_nPhi );
	p_corrAvg = new array2D( p_nQ1, p_nPhi );
}

//--------------
// Destructor --
//--------------
correlate::~correlate ()
{
	delete io;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
correlate::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::beginJob()" );
	MsgLog(name(), info, "calibration dir   = '" << m_calibDir << "'" );
	MsgLog(name(), info, "tifOut            = '" << p_tifOut << "'" );
	MsgLog(name(), info, "edfOut            = '" << p_edfOut << "'" );
	MsgLog(name(), info, "h5Out             = '" << p_h5Out << "'" );
	MsgLog(name(), info, "autoCorrelateOnly = '" << p_autoCorrelateOnly << "'" );
	MsgLog(name(), info, "useBadPixelMask   = '" << p_useBadPixelMask << "'" );
	MsgLog(name(), info, "singleOutput      = '" << p_singleOutput << "'" );
	MsgLog(name(), info, "nPhi              = '" << p_nPhi << "'" );
	MsgLog(name(), info, "nQ1               = '" << p_nQ1 << "'" );
	MsgLog(name(), info, "nQ2               = '" << p_nQ2 << "'" );
	MsgLog(name(), info, "algorithm         = '" << p_alg << "'" );
	MsgLog(name(), info, "startQ            = '" << p_startQ << "'" );
	MsgLog(name(), info, "stopQ             = '" << p_stopQ << "'" );
	MsgLog(name(), info, "LUTx              = '" << p_LUTx << "'" );
	MsgLog(name(), info, "LUTy              = '" << p_LUTy << "'" );
	//MsgLog(name(), info, "   = '" << p_ << "'" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
correlate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "correlate::beginRun()" );
	
	m_cspad_calibpar   = new PSCalib::CSPadCalibPars(m_calibDir, m_typeGroupName, m_src, m_runNumber);
	m_pix_coords_2x1   = new CSPadPixCoords::PixCoords2x1();
	m_pix_coords_quad  = new CSPadPixCoords::PixCoordsQuad( m_pix_coords_2x1,  m_cspad_calibpar, m_tiltIsApplied );
	m_pix_coords_cspad = new CSPadPixCoords::PixCoordsCSPad( m_pix_coords_quad, m_cspad_calibpar, m_tiltIsApplied );
	
	//cout << "------------calib pars---------------" << endl;
	//m_cspad_calibpar->printCalibPars();
	//cout << "------------pix coords 2x1-----------" << endl;
	//m_pix_coords_2x1->print_member_data();


	//get pixel value arrays and eventually a bad pixel mask
	p_pixX = new array1D( m_pix_coords_cspad->getPixCoorArrX_um(), nMaxTotalPx);
	p_pixY = new array1D( m_pix_coords_cspad->getPixCoorArrY_um(), nMaxTotalPx);

	//load bad pixel mask .... TODO 
	p_badPixelMask = NULL;
	
	
	
	if ( p_alg == 2 || p_alg == 4 ){
		//prepare lookup table once in a dummy CrossCorrelator object, so it doesn't have to be done every time
		CrossCorrelator *lutcc = new CrossCorrelator(p_pixX, p_pixX, p_pixY, p_nPhi, p_nQ1);
		lutcc->createLookupTable(p_LUTy, p_LUTx);
		p_LUT = new array2D( *(lutcc->lookupTable()) );
		io->writeToEDF( "LUT.edf", p_LUT );
	}	
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

	shared_ptr<array1D> data = evt.get();
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
			if (p_useBadPixelMask) {											//auto-correlation 2D case, with mask
				cc = new CrossCorrelator( data.get(), p_pixX, p_pixY, p_nPhi, p_nQ1, 0, p_badPixelMask );
			} else { 															//auto-correlation 2D case, no mask
				cc = new CrossCorrelator( data.get(), p_pixX, p_pixY, p_nPhi, p_nQ1 );
			}
		} else {
			if (p_useBadPixelMask){												//full cross-correlation 3D case, with mask
				cc = new CrossCorrelator( data.get(), p_pixX, p_pixY, p_nPhi, p_nQ1, p_nQ2, p_badPixelMask );
			} else { 															//full cross-correlation 3D case, no mask
				cc = new CrossCorrelator( data.get(), p_pixX, p_pixY, p_nQ1, p_nQ1, p_nQ2);
			}
		}
		
		cc->setDebug(1); 

		switch (p_alg) {
			case 1:
				MsgLog(name(), info, "DIRECT COORDINATES, DIRECT XCCA (algorithm 1)");
				cc->calculatePolarCoordinates( p_startQ, p_stopQ );
				cc->calculateSAXS();
				cc->calculateXCCA();	
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
				cc->calculateXCCA();
				break;
			default:
				MsgLog(name(), error,  "Choice of algorithm is invalid. Aborting.");
				return; 
		}//switch

		MsgLog(name(), info, "correlation calculation done. writing (single event) files.");
		if ( p_singleOutput ){
			io->writeToEDF( cc->outputdir()+"corr"+eventname_str+".edf", cc->autoCorr() );
			io->writeToEDF( cc->outputdir()+"polar"+eventname_str+".edf", cc->polar() );
		}
		
		MsgLog(name(), info, "updating running sums.");
		p_polarAvg->addArrayElementwise( cc->polar() );
		p_corrAvg->addArrayElementwise( cc->autoCorr() );
		
		delete io;
		delete cc;
		
	}else{
		MsgLog(name(), info, "could not get CSPAD data" );
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
	
	delete m_cspad_calibpar; 
	delete m_pix_coords_2x1;
	delete m_pix_coords_quad;
	delete m_pix_coords_cspad;
	
	delete p_pixX;
	delete p_pixY;
	delete p_badPixelMask;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
correlate::endJob(Event& evt, Env& env)
{
	//STILL NEED TO CREATE AVERAGES OUT OF THE RUNNING SUMS

	MsgLog(name(), debug,  "correlate::endJob()" );
	
	//TIFF image output
	if (p_tifOut){
		if (p_autoCorrelateOnly){
			io->writeToTiff( "run-xaca.tif", p_polarAvg, 1 );			// 0: unscaled, 1: scaled
			io->writeToTiff( "run-xaca.tif", p_corrAvg, 1 );			// 0: unscaled, 1: scaled
		}else{
			MsgLog(name(), warning, "WARNING. No tiff output for 3D cross-correlation case implemented, yet!" );
			//one possibility would be to write a stack of tiffs, one for each of the outer q values
		}
	}
	//EDF output
	if (p_edfOut){
		if (p_autoCorrelateOnly){
			io->writeToEDF( "run-xaca.edf", p_polarAvg );
			io->writeToEDF( "run-xaca.edf", p_corrAvg );
		}else{
			MsgLog(name(), warning, "WARNING. No EDF output for 3D cross-correlation case implemented, yet!" );
		}
	}
	//HDF5 output
	if (p_h5Out){
		if (p_autoCorrelateOnly){
			io->writeToHDF5( "run-xaca.h5", p_polarAvg);
			io->writeToHDF5( "run-xaca.h5", p_corrAvg );
		}else{
			MsgLog(name(), warning, "WARNING. No HDF5 output for 3D cross-correlation case implemented, yet!" );
		}
	}
}

} // namespace kitty
