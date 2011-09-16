#ifndef KITTY_CORRELATE_H
#define KITTY_CORRELATE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class correlate.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------

//----------------------
// Base Class Headers --
//----------------------
#include "psana/Module.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "kitty/arrayclasses.h"
#include "kitty/arraydataIO.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------
#include "PSCalib/CSPadCalibPars.h"

#include "CSPadPixCoords/QuadParameters.h"
#include "CSPadPixCoords/PixCoords2x1.h"
#include "CSPadPixCoords/PixCoordsQuad.h"
#include "CSPadPixCoords/PixCoordsCSPad.h"


//		---------------------
// 		-- Class Interface --
//		---------------------

namespace kitty {

/// @addtogroup kitty

/**
 *  @ingroup kitty
 *
 *  @brief Example module class for psana
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @see AdditionalClass
 *
 *  @version \$Id$
 *
 *  @author Jan Moritz Feldkamp
 */

class correlate : public Module {
public:

	// Default constructor
	correlate (const std::string& name) ;

	// Destructor
	virtual ~correlate () ;

	/// Method which is called once at the beginning of the job
	virtual void beginJob(Event& evt, Env& env);

	/// Method which is called at the beginning of the run
	virtual void beginRun(Event& evt, Env& env);

	/// Method which is called at the beginning of the calibration cycle
	virtual void beginCalibCycle(Event& evt, Env& env);

	/// Method which is called with event data, this is the only required 
	/// method, all other methods are optional
	virtual void event(Event& evt, Env& env);

	/// Method which is called at the end of the calibration cycle
	virtual void endCalibCycle(Event& evt, Env& env);

	/// Method which is called at the end of the run
	virtual void endRun(Event& evt, Env& env);

	/// Method which is called once at the end of the job
	virtual void endJob(Event& evt, Env& env);

protected:

private:

	int p_tifOut;
	int p_edfOut;
	int p_h5Out;

	int p_autoCorrelateOnly;
	int p_useBadPixelMask;
	int p_singleOutput;
	
	int p_nPhi;
	int p_nQ1;
	int p_nQ2;
	
	int p_alg;
	
	int p_startQ;
	int p_stopQ;
	
	int p_LUTx;
	int p_LUTy;
	
	arraydataIO *io;
	
	array1D *p_pixX;
	array1D *p_pixY;
	array1D *p_badPixelMask;

	array2D *p_LUT;

	array2D *p_polarAvg;
	array2D *p_corrAvg;
	
	//needed for CSPadPixCoords
	bool			m_tiltIsApplied;
	
	//these four are needed for CSPadCalibPars (parser of the calibration data)
	std::string 	m_calibDir;       	// i.e. /reg/d/psdm/CXI/cxi35711/calib
	std::string		m_typeGroupName;  	// i.e. CsPad::CalibV1
	std::string		m_src;         		// Data source set from config file
	unsigned 		m_runNumber;
		
	PSCalib::CSPadCalibPars        *m_cspad_calibpar;		//all calibration information
	CSPadPixCoords::PixCoords2x1   *m_pix_coords_2x1;		//pixel coordinates for 2x1s
	CSPadPixCoords::PixCoordsQuad  *m_pix_coords_quad;		//pixel coordinates for quads
	CSPadPixCoords::PixCoordsCSPad *m_pix_coords_cspad;		//pixel coordinates for whole CSPAD
};

} // namespace kitty

#endif // KITTY_CORRELATE_H
