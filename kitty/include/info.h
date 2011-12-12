#ifndef KITTY_INFO_H
#define KITTY_INFO_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class info.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <fstream>
#include <vector>

//----------------------
// Base Class Headers --
//----------------------
#include "psana/Module.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//headers must be available through symbolic links or copies in the kitty include directory
#include "kitty/constants.h"

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

class info : public Module {
public:

	// Default constructor
	info (const std::string& name) ;

	// Destructor
	virtual ~info () ;

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
	std::string	p_outputPrefix;
	int p_count;
	bool p_stopflag;
	
	std::ofstream fout;
	
	std::map<std::string, pv> p_pvs;					// container for list of PVs

};

} // namespace kitty

#endif // KITTY_info_H
