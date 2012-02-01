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
#include "kitty/util.h"
using ns_cspad_util::create1DFromRawImageCSPAD;
using ns_cspad_util::createAssembledImageCSPAD;
using ns_cspad_util::createRawImageCSPAD;


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

//define what values in the mask is interpreted as good and bad pixels
const int GOOD = 1;
const int BAD = 0; 

//----------------
// Constructors --
//----------------
makemask::makemask (const std::string& name)
	: Module(name)
	, p_badPixelLowerBoundary(0)
	, p_badPixelUpperBoundary(0)
	, p_outputPrefix("")
	, p_mask_fn("")
	, p_useMask(0)
	, p_takeOutThirteenthRow(0)
	, p_takeOutASICFrame(0)
	, p_takeOutWholeSection(-1)
	, io(0)
	, p_mask(0)
	, p_sum_sp()
	, p_count(0)
{
  // get the values from configuration or use defaults
	
	p_badPixelLowerBoundary	= config   ("badPixelLowerBoundary", 	0);
	p_badPixelUpperBoundary	= config   ("badPixelUpperBoundary", 	1500);
	p_mask_fn				= configStr("mask", 					"");
	p_useMask				= config   ("useMask", 					0);
	p_takeOutThirteenthRow	= config   ("takeOutThirteenthRow", 	1);
	p_takeOutASICFrame		= config   ("takeOutASICFrame", 		1);
	p_takeOutWholeSection		= config   ("takeOutWholeSection", 		-1);
			
	io = new arraydataIO();
	p_sum_sp = shared_ptr<array1D<double> >( new array1D<double>(nMaxTotalPx) );
}

//--------------
// Destructor --
//--------------
makemask::~makemask ()
{
	delete io;
	delete p_mask;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
makemask::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginJob()" );
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
			p_mask = new array1D<double>;
						
			array2D<double> *img2D = 0;
			int fail = io->readFromFile( p_mask_fn, img2D );
			create1DFromRawImageCSPAD( img2D, p_mask );
			delete img2D;
			
			
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
	MsgLog(name(), debug,  "beginRun()" );
	
	p_outputPrefix = *( (shared_ptr<std::string>) evt.get(IDSTRING_OUTPUT_PREFIX) ).get();
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
makemask::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
makemask::event(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "event()" );
	shared_ptr<array1D<double> > data_sp = evt.get(IDSTRING_CSPAD_DATA);
	
	if (data_sp){
		p_sum_sp->addArrayElementwise( data_sp.get() );	
		p_count++;
	}
}
  
  
/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
makemask::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
makemask::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
makemask::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endJob()" );
	
	array1D<double> *mask = NULL;
	if (p_useMask){
		//allocate a mask (start from the one already read from file)
		mask = new array1D<double>( p_mask );
	}else{
		//allocate a mask. Initially, all pixels are GOOD
		mask = new array1D<double>( nMaxTotalPx );
		for (unsigned int i = 0; i<p_sum_sp->size(); i++){
			mask->set(i, GOOD);
		}
	}

	if ( p_count != 0 ){	
		//create average out of raw sum
		p_sum_sp->divideByValue( p_count );

		//	MsgLog(name(), info, "---complete histogram of averaged data---\n" << p_sum_sp->getHistogramASCII(50) );
		MsgLog(name(), info, "---partial histogram of averaged data---\n" << p_sum_sp->getHistogramInBoundariesASCII(100, -100, 400) );
			
		//set mask to 1 if pixel is valid, otherwise, leave 0
		for (unsigned int i = 0; i<p_sum_sp->size(); i++){
			if ( (p_sum_sp->get(i) < p_badPixelLowerBoundary) || (p_sum_sp->get(i) > p_badPixelUpperBoundary) ){
				//pixel seems bad: exclude this pixel
				mask->set(i, BAD);
			}
		}
	}else{
		MsgLog( name(), warning, "No events. No data-dependent masking (thresholding) possible." );
	}
			
	//go through data again: analytically take out certain areas
	//   - a whole ASIC, if specified in config file
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
						mask->set( index, BAD );
					}//if takeOutASICFrame
					
					if ( p_takeOutThirteenthRow && 
						(r==nRowsPerASIC-13 || r==2*nRowsPerASIC-13) ){
							mask->set( index, BAD );
					}//if takeOutThirteenthRow
					
					if ( p_takeOutWholeSection == q*nMax2x1sPerQuad+s ){
						mask->set( index, BAD );
					}//if takeOutWholeSection
					
				}//for r
			}//for c
		}//for s
	}//for q
	
	MsgLog(name(), info, "---histogram of mask---\n" << mask->getHistogramASCII(2) );
	
	//output 2D
	array2D<double> *mask2D = new array2D<double>;
	createRawImageCSPAD( mask, mask2D );
	string ext = ".h5";
	io->writeToFile( p_outputPrefix+"_mask_raw2D"+ext, mask2D );
	delete mask2D;
	
	//output 1D
	//io->writeToEDF( p_outputPrefix+"_mask_1D.edf", mask );	
	
	delete mask;
}



} // namespace kitty
