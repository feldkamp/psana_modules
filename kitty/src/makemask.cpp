//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class makemask...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/makemask.h"

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
PSANA_MODULE_FACTORY(makemask)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
makemask::makemask (const std::string& name)
  : Module(name)
{
  // get the values from configuration or use defaults
	
	p_badPixelLowerBoundary	= config   ("badPixelLowerBoundary", 	0);
	p_badPixelUpperBoundary	= config   ("badPixelUpperBoundary", 	1500);
	p_outputPrefix			= configStr("outputPrefix", 			"");
	p_mask_fn				= configStr("mask", 					"");
	p_useMask				= config   ("useMask", 					0);
	p_takeOutThirteenthRow	= config   ("takeOutThirteenthRow", 	1);
	p_takeOutASICFrame		= config   ("takeOutASICFrame", 		1);
		
	io = new arraydataIO;
	p_sum = new array1D( nMaxTotalPx );
	p_mask = new array1D();
	
	p_count = 0;
}

//--------------
// Destructor --
//--------------
makemask::~makemask ()
{
	delete io;
	delete p_sum;
	delete p_mask;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
makemask::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "makemask::beginJob()" );
	MsgLog(name(), info, "badPixelLowerBoundary = '" << p_badPixelLowerBoundary << "'" );	
	MsgLog(name(), info, "badPixelUpperBoundary = '" << p_badPixelUpperBoundary << "'" );
	MsgLog(name(), info, "mask file = '" << p_mask_fn << "'" );
	MsgLog(name(), info, "use mask correction = '" << p_useMask << "'" );
	MsgLog(name(), info, "takeOutThirteenthRow = '" << p_takeOutThirteenthRow << "'" );
	MsgLog(name(), info, "takeOutASICFrame = '" << p_takeOutASICFrame << "'" );


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
makemask::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "makemask::beginRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
makemask::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "makemask::beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
makemask::event(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "makemask::event()" );

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
makemask::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "makemask::endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
makemask::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "makemask::endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
makemask::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "makemask::endJob()" );

	//create average out of raw sum
	p_sum->divideByValue( p_count );
	
	array1D *mask = NULL;
	if (p_useMask){
		//allocate a mask (start from the one already read from file)
		mask = new array1D( p_mask );
	}else{
		//allocate a mask (initially 1 everywhere)
		mask = new array1D( nMaxTotalPx );
		mask->ones();
	}
	
	//set mask to 1 if pixel is valid, otherwise, leave 0
	for (unsigned int i = 0; i<p_sum->size(); i++){
		if ( (p_sum->get(i) < p_badPixelLowerBoundary) || (p_sum->get(i) > p_badPixelUpperBoundary) ){
			//pixel seems bad: change mask and exclude this pixel
			mask->set(i, 0);
		}else{
			//pixel seems valid: do not change mask
		}
	}
	
	//go through data again: analytically take out certain areas
	//   - first and last row on each ASIC
	//   - first and last column on each ASIC
	//   - row 13 on each ASIC
	for (int q = 0; q < nMaxQuads; q++){
		for (int s = 0; s < nMax2x1sPerQuad; s++){
			for (int c = 0; c < nColsPer2x1; c++){
				for (int r = 0; r < nRowsPer2x1; r++){
					int index = q*nMaxPxPerQuad + s*nPxPer2x1 + c*nRowsPer2x1 + r;
					if ( p_takeOutASICFrame && 
						(r==0 || r==nRowsPerASIC-1 || r==nRowsPerASIC || r==2*nRowsPerASIC-1 || c==0 || c==nColsPerASIC-1) ){
						mask->set( index, 0 );
					}//if
					if ( p_takeOutThirteenthRow && 
						(r==nRowsPerASIC-13 || r==2*nRowsPerASIC-13) ){
							mask->set( index, 0 );
					}//if
				}//for r
			}//for c
		}//for s
	}//for q
	
	
	io->writeToEDF( p_outputPrefix+"_1D.edf", mask );
	
	array2D* mask2D = new array2D;
	mask2D->createRawImageCSPAD( mask );
	io->writeToEDF( p_outputPrefix+"_raw2D.edf", mask2D );
	delete mask2D;
	

	MsgLog(name(), info, "---histogram of mask---\n" << mask->getHistogramASCII(2) );
//	MsgLog(name(), info, "---complete histogram of averaged data---\n" << p_sum->getHistogramASCII(50) );
	MsgLog(name(), info, "---partial histogram of averaged data---\n" << p_sum->getHistogramInBoundariesASCII(100, -100, 400) );
	
	delete mask;
}



} // namespace kitty
