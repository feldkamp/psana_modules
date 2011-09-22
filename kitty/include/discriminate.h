#ifndef KITTY_DISCRIMINATE_H
#define KITTY_DISCRIMINATE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class discriminate.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <vector>

//----------------------
// Base Class Headers --
//----------------------
#include "psana/Module.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//headers must be available through symbolic links or copies in the kitty include directory
#include <kitty/arrayclasses.h>

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

class discriminate : public Module {
public:

	// Default constructor
	discriminate (const std::string& name) ;

	// Destructor
	virtual ~discriminate () ;

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
	//own private variables
	int p_lowerThreshold;
	int p_upperThreshold;
	
	int p_skipcount;
	int p_hitcount;
	
	int p_numQuads;
	int p_numSect;
	
	std::vector<double> p_hitInt;
	
	int p_count;
	int p_maxHits;
	
	int p_useShift;
	double p_shiftX;
	double p_shiftY;

	//---------------------------------------------------------------pdsm standard stuff
	//needed to read data from detector
	std::string		m_src;         		// Data source set from config file, i.e. CxiDs1.0:Cspad.0
	Pds::Src		m_actualSrc;
	
	//needed for CSPadCalibPars (parser of the calibration data)
	std::string 	m_calibDir;       	// i.e. /reg/d/psdm/CXI/cxi35711/calib
	std::string		m_typeGroupName;  	// i.e. CsPad::CalibV1
	unsigned		m_runNumber;
	bool			m_tiltIsApplied;
		
	PSCalib::CSPadCalibPars        *m_cspad_calibpar;		//all calibration information
	CSPadPixCoords::PixCoords2x1   *m_pix_coords_2x1;		//pixel coordinates for 2x1s
	CSPadPixCoords::PixCoordsQuad  *m_pix_coords_quad;		//pixel coordinates for quads
	CSPadPixCoords::PixCoordsCSPad *m_pix_coords_cspad;		//pixel coordinates for whole CSPAD	
};

} // namespace kitty

#endif // KITTY_DISCRIMINATE_H
