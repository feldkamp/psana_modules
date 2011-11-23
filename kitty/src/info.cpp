//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class info...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/info.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <iomanip>
using std::setw;

#include <sstream>

#include <vector>
using std::vector;

#include <string>
using std::string;

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
PSANA_MODULE_FACTORY(info)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
info::info (const std::string& name)
	: Module(name)
	, p_outputPrefix("")
	, p_count(0)
	, p_stopflag(false)
	, fout()
	, runPVs()
	, runDesc()
{
	p_outputPrefix				= configStr("outputPrefix", 		"out");
}

//--------------
// Destructor --
//--------------
info::~info ()
{
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
info::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginJob()" );
	
	runPVs.push_back("CXI:DS1:MMS:06");									//# 0
	runDesc.push_back("detector_position");
	
	runPVs.push_back("BEND:DMP1:400:BDES");								//# 1
	runDesc.push_back("electron_beam_energy");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO289");							//# 2
	runDesc.push_back("Vernier_energy");
	
	runPVs.push_back("BPMS:DMP1:199:TMIT1H");							//# 3
	runDesc.push_back("particle_N_electrons");

	runPVs.push_back("EVNT:SYS0:1:LCLSBEAMRATE");						//# 4
	runDesc.push_back("LCLS_repetition_rate_Hz");
		
	runPVs.push_back("SIOC:SYS0:ML00:AO195");							//# 5
	runDesc.push_back("peak_current_after_second_bunch_compressor");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO820");							//# 6
	runDesc.push_back("pulse_length");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO569");							//# 7
	runDesc.push_back("ebeam_energy_loss_converted_to_photon_mJ");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO580");							//# 8
	runDesc.push_back("calculated_number_of_photons");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO541");							//# 9
	runDesc.push_back("photon_beam_energy_eV");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO627");							//# 10
	runDesc.push_back("photon_beam_energy");
	
	runPVs.push_back("SIOC:SYS0:ML00:AO192");							//# 11
	runDesc.push_back("wavelength_nm");


	ostringstream header;
	header << "run ";
	for (unsigned int i = 0; i < runPVs.size(); i++){
		header << runPVs.at(i) << " ";
	}
	header << endl;
	header << "run ";
	for (unsigned int i = 0; i < runPVs.size(); i++){
		header << runDesc.at(i) << " ";
	}
	header << endl;
			
	std::ofstream headerfile("header.txt");
	headerfile << header.str() << endl;
}




/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
info::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "beginRun()" );

	bool displayAllAvailablePVs = true;

	ostringstream runinfo;

	//get run rumber for this run
	shared_ptr<EventId> eventId = evt.get();
	int runNumber = 0;
    if (eventId.get()) {
		runNumber = eventId->run();
		MsgLog(name(), trace, name() << ": Using run number " << runNumber);
		runinfo << runNumber <<m " ";
    } else {
		MsgLog(name(), warning, name() << ": Cannot get eventId.");
    }	

	ostringstream fnosst;
	fnosst << p_outputPrefix << "_r" << runNumber << ".txt";
	fout.open(fnosst.str().c_str());
	if (fout.fail()){
		MsgLog(name(), error,  "could not open output file " << fnosst.str());
	}
	
	if (displayAllAvailablePVs){		
		const vector<string>& pvNames = env.epicsStore().pvNames();
		if (pvNames.size() == 0){
			MsgLog(name(), info, " --- no stored PVs --- " );
		}else{
			MsgLog(name(), info, "------list of stored PVs------" );
			for (unsigned int i = 0; i < pvNames.size(); i++){
				MsgLog(name(), info, pvNames.at(i) );
			}
		}
	}
	
	MsgLog(name(), info, "------list of read out PVs------" );
		
//	unsigned int detZ_no = 0;
//	unsigned int lambda_no = 11;	
	
	vector<double> runVals;						// vector for corresponding values
	runVals.assign(runPVs.size(), 0.);			// set all values of PVs to zero initially
	vector<bool> runPVvalid;					// keep track of the validity of this PV
	runPVvalid.assign(runPVs.size(), false);
	
	//try to get the PV values
	for (unsigned int i = 0; i < runPVs.size(); i++){
		try{
			runVals.at(i) = env.epicsStore().value(runPVs.at(i));
			MsgLog(name(), info, setw(3) << "#" << i << " " 
				<< setw(26) << runPVs.at(i) << " = " 
				<< setw(12) << runVals.at(i) 
				<< " (" << runDesc.at(i) << ")" );
			runPVvalid.at(i) = true;
		}catch(...){
			MsgLog(name(), info, setw(3) << "#" << i << " " 
				<< setw(26) << runPVs.at(i) << " = "
				<< setw(12) << " ---- " 
				<< " (" << runDesc.at(i) << ")" );
			runPVvalid.at(i) = false;
		}
	}	
	
	//use the values
	MsgLog(name(), info, "run no " << runNumber);
	for (unsigned int i = 0; i < runPVs.size(); i++){
		runinfo << runVals.at(i) << " ";
	}
	runinfo << endl;
	
	fout << runinfo.str();
	
	
//	MsgLog(name(), info, "------quantities derived from selected PVs------" );
//	double detZ = 0; 
//	if (runPVvalid.at(detZ_no) ){
//		detZ = detOffset + runVals.at(detZ_no);
//	}
//	double lambda = 0;
//	if (runPVvalid.at(lambda_no) ){
//		lambda = runVals.at(lambda_no);
//	}
//	double two_k = 2*2*M_PI/lambda;								//in units of inv. nanometers
//	MsgLog(name(), info, "detZ = " << detZ << " mm");				
//	MsgLog(name(), info, "2*k = " << two_k << "nm^-1" );
//	MsgLog(name(), info, " ");
	
	std::list<EventKey> keys = evt.keys();
	MsgLog(name(), info, "------" << keys.size() << " keys in event------");
	for ( std::list<EventKey>::iterator it = keys.begin(); it != keys.end(); it++ ){
		ostringstream out;
		it->print(out);
		MsgLog(name(), info, out.str() << ", key: '" << it->key() << "'");
	}
	



	
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
info::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
info::event(Event& evt, Env& env)
{
	p_stopflag = true;

	if (p_stopflag){
		stop();
		return;
	}
	
	//compile a list of PVs that need to be read out for each event 
	// (works, but commented out since it's not needed right now)
//	vector<string> eventPVs;						// vector for list of PVs
//	eventPVs.push_back("BEAM:LCLS:ELEC:Q");
//	
//	vector<double> eventVals;						// vector for corresponding values
//	eventVals.assign(eventPVs.size(), 0.);			// set all to zero
//	
//	for (unsigned int i = 0; i < eventPVs.size(); i++){
//		try{
//			eventVals.at(i) = env.epicsStore().value(eventPVs.at(i));
//			MsgLog(name(), info, eventPVs.at(i) << " = " << eventVals.at(i) );
//		}catch(...){
//			MsgLog(name(), warning, "PV " << eventPVs.at(i) << " doesn't exist." );
//		}
//	}

	
	// increment event counter
	p_count++;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
info::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
info::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
info::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endJob()" );

	fout.close();
}



} // namespace kitty
