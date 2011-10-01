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
	std::string p_mask_fn;
	int p_useMask;
	int p_singleOutput;
	
	int p_nPhi;
	int p_nQ1;
	int p_nQ2;
	int p_nLag;
	
	int p_alg;
	
	int p_startQ;
	int p_stopQ;
	
	int p_LUTx;
	int p_LUTy;
	
	std::string p_outputPrefix;
	
	arraydataIO *io;
	
	shared_ptr<array1D> p_pixX_sp;
	shared_ptr<array1D> p_pixY_sp;
	
	array1D *p_mask;

	array2D *p_LUT;

	array2D *p_polarAvg;
	array2D *p_corrAvg;
	
	array1D *p_qAvg;
	array1D *p_iAvg;
	
	int p_count;

};

} // namespace kitty

#endif // KITTY_CORRELATE_H
