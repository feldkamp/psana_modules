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
#include <map>
#include <set>

//----------------------
// Base Class Headers --
//----------------------
#include "psana/Module.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//headers must be available through symbolic links or copies in the kitty include directory
#include <kitty/constants.h>
#include <kitty/arrayclasses.h>
#include <kitty/arraydataIO.h>

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


	//create the internal pixel arrays
	void makePixelArrays();	
	
	//add the calibration arrays to the event for other modules to use
	void addPixelArraysToEvent( Event &evt );

protected:

private:
	int p_useCorrectedData;
	double p_lowerThreshold;
	double p_upperThreshold;
	
	int p_discriminateAlogrithm;			// selects, which discrimination algorithm is used
	std::string p_hitlist_fn;
	std::set<unsigned int> p_hitlist;		// p_hitlist has two functions: 
											// 1.) in regular operation: keeps track of which shots are hits, based on hit finding
											// 2.) in listfinder operation: reads file in hitlist_fn to tell 'discriminate' which shots to accept
	
	std::string	p_outputPrefix;
	int p_pixelVectorOutput;
	
	int p_maxHits;
	int p_skipcount;
	int p_hitcount;
	int p_count;
	int p_makePixelArrays_count;
	// if g_stopflag is set to true, execution will stop as soon as the beginning of the next event
	bool p_stopflag;	

	int p_useShift;
	double p_shiftX;
	double p_shiftY;
	double p_detOffset;				// default = 500.0 + 63.0 = 563
	double p_detDistance;			// detector distance from sample
	double p_lambda;				// wavelength
	bool p_criticalPVchange;		// if true, let the other modules know to update all pv-dependent properties in their event()
	std::map<std::string,pv> p_pvs;		// container for list of PVs

	
	//-------------comment on detOffset------------------------------------------
	// At the closest possible position, the stage is at -500mm
	// By design, that's 63mm away from interaction region (cxi35711 elog post 42531)
	// Further calibration is required for more accuracy
	//     |                     ||               ............. ||
	//     |                     || .............               ||
	//     v        .............||                             ||
	// --> X - - - - - - - - - - || - - - - - - - - - - - - - - ||
	//              .............||                             ||
	//                           || .............               ||
	//                           ||               ............. ||
	//     jet                   close pos                      far pos
	

	std::vector<double> p_hitInt;		// vector to keep track of the hit intensity	

	shared_ptr<array1D<double> > p_pixX_um_sp;
	shared_ptr<array1D<double> > p_pixY_um_sp;
	shared_ptr<array1D<double> > p_pixX_int_sp;
	shared_ptr<array1D<double> > p_pixY_int_sp;
	shared_ptr<array1D<double> > p_pixX_pix_sp;
	shared_ptr<array1D<double> > p_pixY_pix_sp;
	shared_ptr<array1D<double> > p_pixX_q_sp;
	shared_ptr<array1D<double> > p_pixY_q_sp;
	shared_ptr<array1D<double> > p_pixTwoTheta_sp;
	shared_ptr<array1D<double> > p_pixPhi_sp;
	
	unsigned int	p_runNumber;	// stores the current run number (updated in beginRun())
	
	//---------------------------------------------------------------pdsm standard stuff
	//needed to read data from detector
	std::string		m_dataSourceString;			// Data source set from config file, i.e. CxiDs1.0:Cspad.0
	
	//needed for CSPadCalibPars (parser of the calibration data)
	std::string		m_calibSourceString;
	std::string 	m_calibDir;       	// i.e. /reg/d/psdm/CXI/cxi35711/calib
	std::string		m_typeGroupName;  	// i.e. CsPad::CalibV1
	bool			m_tiltIsApplied;
	
	PSCalib::CSPadCalibPars        *m_cspad_calibpar;		//all calibration information
	CSPadPixCoords::PixCoords2x1   *m_pix_coords_2x1;		//pixel coordinates for 2x1s
	CSPadPixCoords::PixCoordsQuad  *m_pix_coords_quad;		//pixel coordinates for quads
	CSPadPixCoords::PixCoordsCSPad *m_pix_coords_cspad;		//pixel coordinates for whole CSPAD	
};

} // namespace kitty

#endif // KITTY_DISCRIMINATE_H
