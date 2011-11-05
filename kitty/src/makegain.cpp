//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class makegain...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/makegain.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <cmath>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "PSEvt/EventId.h"

#include "kitty/constants.h"
#include "kitty/util.h"
#include "kitty/crosscorrelator.h"


//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

// This declares this class as psana module
using namespace kitty;
PSANA_MODULE_FACTORY(makegain)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
makegain::makegain (const std::string& name)
	: Module(name)
	, p_model_fn("")
	, p_modelDelta(0.)
	, p_outputPrefix("")
	, io(0)
	, p_model(0)
	, p_pixX_q_sp()
	, p_pixY_q_sp()
	, p_pixX_int_sp()
	, p_pixY_int_sp()
	, p_count(0)
{
	p_model_fn				= configStr("model", 					"");
	p_modelDelta			= config   ("modelDelta",				1.);
		
	io = new arraydataIO();
}

//--------------
// Destructor --
//--------------
makegain::~makegain ()
{
	delete io;
	delete p_model;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
makegain::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginJob()" );
	MsgLog(name(), info, "model file = '" << p_model_fn << "'" );
	MsgLog(name(), info, "model delta = '" << p_modelDelta << "'" );
	
	if (p_model_fn != ""){
		int fail = io->readFromASCII( p_model_fn, p_model );
		if (fail){
			MsgLog(name(), error, "Could not read model. Aborting...");
			return;
		}else{
			MsgLog(name(), info, "model read. min:" << p_model->calcMin() << ", max:" << p_model->calcMax() );
		}
	}else{
			MsgLog(name(), warning, "No model file specified in config file. Aborting...");
			return;
	}
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
makegain::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginRun()" );

	//get calibration information	
	p_pixX_q_sp = evt.get(IDSTRING_PX_X_q);
	p_pixY_q_sp = evt.get(IDSTRING_PX_Y_q);
	p_pixX_int_sp = evt.get(IDSTRING_PX_X_int);
	p_pixY_int_sp = evt.get(IDSTRING_PX_Y_int);
	
	p_outputPrefix = *( (shared_ptr<std::string>) evt.get(IDSTRING_OUTPUT_PREFIX) ).get();
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
makegain::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
makegain::event(Event& evt, Env& env)
{
	MsgLog(name(), debug, "event()" );
}
  
  
/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
makegain::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug, "endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
makegain::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug, "endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
makegain::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "endJob()" );

	shared_ptr<array1D> avg_sp = evt.get(IDSTRING_CSPAD_RUNAVGERAGE);
	
	//find out maximum of angular average (by using that function from cross correlator)
	int nPhi = 256;
	int nQ1 = 500;
	int startQ = 0;
	int stopQ = 1000;
	int alg = 1;
	bool calcSAXS = true;
	
	CrossCorrelator *cc = new CrossCorrelator( avg_sp.get(), p_pixY_int_sp.get(), p_pixX_int_sp.get(), nPhi, nQ1 );
	cc->run(startQ, stopQ, alg, calcSAXS);
	
	array1D *SAXS = 0;
	cc->polar()->calcAvgCol( SAXS );
	
	io->writeToHDF5( p_outputPrefix+"_saxs_1D.h5", SAXS );
	io->writeToHDF5( p_outputPrefix+"_saxs_2D.h5", cc->polar() );
	double datamax = SAXS->calcMax();
	
	//scale model to the intensity found in the scattering data
	double modelmax = p_model->calcMax();
	double scaling = datamax/modelmax;
	p_model->multiplyByValue(scaling);
	MsgLog(name(), info,  "model scaled to: " << scaling << " = " << datamax << "/" << modelmax);
	io->writeToHDF5( p_outputPrefix+"_model_1D.h5", p_model );
	
	
	array1D *gain = new array1D(nMaxTotalPx);
	array1D *expected = new array1D(nMaxTotalPx);
	
	//calculate the expected signal for each q-value
	//and create a gainmap from the discrepancy
	for (unsigned int i=0; i < avg_sp->size(); i++){
		//find absolute q-value for this data point
		double qx = p_pixX_q_sp->get(i);
		double qy = p_pixY_q_sp->get(i);
		double q_abs = sqrt( qx*qx + qy*qy );
		//compare to model
		int q_model_index = (int) floor( q_abs/p_modelDelta ); 
		double correction = avg_sp->get(i) / p_model->get(q_model_index);
		gain->set(i, correction);
		
		expected->set(i, p_model->get(q_model_index));
		if (i > 10000 && i < 10010){
			cout << "i=" << i << ", qx=" << qx << ", qy=" << qy << ", |q|=" << q_abs 
				<< ", avg_sp(i)=" << avg_sp->get(i) << endl;
			cout << "   model index=" << q_model_index << ", p_model->get(index)=" << p_model->get(q_model_index) 
				<< ", correction=" << correction << endl;
		}
	}
	
	//assemble ASICs
	array2D *model_asm2D = 0;
	createAssembledImageCSPAD( expected, p_pixX_int_sp.get(), p_pixY_int_sp.get(), model_asm2D );	
	io->writeToHDF5( p_outputPrefix+"_model_asm2D.h5", model_asm2D );
	
	//output of 1D EDF to be read later (by the correct module, for instance)
	io->writeToEDF( p_outputPrefix+"_gain_1D.edf", gain );


	//output of 2D raw image (cheetah-style)
	array2D *gain_raw2D = 0;
	createRawImageCSPAD( gain, gain_raw2D );
	io->writeToHDF5( p_outputPrefix+"_gain_raw2D.h5", gain_raw2D );

	//assemble ASICs
	array2D *gain_asm2D = 0;
	createAssembledImageCSPAD( gain, p_pixX_int_sp.get(), p_pixY_int_sp.get(), gain_asm2D );
	io->writeToHDF5( p_outputPrefix+"_gain_asm2D.h5", gain_asm2D );

	delete cc;	
	delete gain;
	delete expected;
}



} // namespace kitty
