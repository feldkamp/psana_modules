#ifndef KITTY_CORRECT_H
#define KITTY_CORRECT_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class correct.
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

class correct : public Module {
public:

	// Default constructor
	correct (const std::string& name) ;

	// Destructor
	virtual ~correct () ;

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
	
	std::string p_back_fn;
	int p_useBack;
	std::string p_gain_fn;
	int p_useGain;
	std::string p_mask_fn;
	int p_useMask;
	std::string p_outputPrefix;
	
	
	arraydataIO *io;
	array1D *p_sum;
	array1D *p_back;
	array1D *p_gain;
	array1D *p_mask;

	int p_count;
};

} // namespace kitty

#endif // KITTY_CORRECT_H
