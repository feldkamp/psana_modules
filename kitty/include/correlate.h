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
#include "kitty/crosscorrelator.h"

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


	//update the internal pixel arrays with information from the event object
	void updatePixelArrays(Event& evt);

protected:

private:

	int p_tifOut;
	int p_edfOut;
	int p_h5Out;
	int p_singleOutput;
	std::string p_outputPrefix;
	
	std::string p_mask_fn;
	int p_useMask;	
	
	int p_nPhi;
	int p_nQ1;
	int p_nQ2;
	int p_nLag;
	
	int p_alg;
	int p_autoCorrelateOnly;
	
	double p_startQ;
	double p_stopQ;
	int p_units;
	
	int p_LUTx;
	int p_LUTy;
	
	arraydataIO *io;
	
	shared_ptr<array1D<double> > p_pix1_sp;		//input vectors for crosscorrelator
	shared_ptr<array1D<double> > p_pix2_sp;
	
	array1D<double> *p_mask;

	shared_ptr<array2D<double> > p_polarAvg_sp;
	shared_ptr<array2D<double> > p_corrAvg_sp;
	
	shared_ptr<array1D<double> > p_qAvg_sp;
	shared_ptr<array1D<double> > p_iAvg_sp;
	
	CrossCorrelator *p_cc;
	
	int p_count;
};

} // namespace kitty

#endif // KITTY_CORRELATE_H
