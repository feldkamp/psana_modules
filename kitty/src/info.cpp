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
	, p_pvs()
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
	
	p_pvs[pvDetPos].name = "CXI:DS1:MMS:06";
	p_pvs[pvDetPos].desc = "detector position";

	p_pvs[pvLambda].name = "SIOC:SYS0:ML00:AO192";
	p_pvs[pvLambda].desc = "wavelength [nm]";
		
	p_pvs[pvElEnergy].name = "BEND:DMP1:400:BDES";
	p_pvs[pvElEnergy].desc = "electron beam energy";
	
	p_pvs[pvNElectrons].name = "BPMS:DMP1:199:TMIT1H";
	p_pvs[pvNElectrons].desc = "particle N_electrons";

	p_pvs[pvRepRate].name = "EVNT:SYS0:1:LCLSBEAMRATE";
	p_pvs[pvRepRate].desc = "LCLS repetition rate [Hz]";
		
	p_pvs[pvPeakCurrent].name = "SIOC:SYS0:ML00:AO195";
	p_pvs[pvPeakCurrent].desc = "peak current after second bunch compressor";
	
	p_pvs[pvPulseLength].name = "SIOC:SYS0:ML00:AO820";
	p_pvs[pvPulseLength].desc = "pulse length";
	
	p_pvs[pvEbeamLoss].name = "SIOC:SYS0:ML00:AO569";
	p_pvs[pvEbeamLoss].desc = "ebeam energy loss converted to photon mJ";
	
	p_pvs[pvNumPhotons].name = "SIOC:SYS0:ML00:AO580";
	p_pvs[pvNumPhotons].desc = "calculated number of photons";
	
	p_pvs[pvPhotonEnergy].name = "SIOC:SYS0:ML00:AO541";
	p_pvs[pvPhotonEnergy].desc = "photon beam energy [eV]";

	//initialize the value and valid fields of the pv structs in the map
	for (map<string,pv>::iterator it = p_pvs.begin(); it != p_pvs.end(); it++){
		it->second.value = 0.;
		it->second.valid = false;
		it->second.changed = false;
	} 

	ostringstream header;
	header << "run ";
	map<string,pv>::iterator it;
	for(it = p_pvs.begin(); it != p_pvs.end(); it++){
		header << it->second.name << " ";
	}
	header << endl;
	header << "run ";
	for(it = p_pvs.begin(); it != p_pvs.end(); it++){
		header << it->second.desc << " ";
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
		runinfo << runNumber << " ";
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
		const vector<string>& all_pvNames = env.epicsStore().pvNames();
		if (all_pvNames.size() == 0){
			MsgLog(name(), info, " --- no stored PVs --- " );
		}else{
			MsgLog(name(), info, "------list of stored PVs------" );
			for (unsigned int i = 0; i < all_pvNames.size(); i++){
				MsgLog(name(), info, all_pvNames.at(i) );
			}
		}
	}

	//try to get the PV values
	map<string,pv>::iterator it;
	ostringstream osst;
	for (it = p_pvs.begin(); it != p_pvs.end(); it++){
		osst << setw(20) << it->first << " " << setw(26) << it->second.name; 
		try{
			it->second.value = env.epicsStore().value(it->second.name);
			it->second.valid = true;
			osst << " = " << setw(12) << it->second.value;
		}catch(...){
			it->second.valid = false;
			osst << setw(12) << " ---- ";
		}
		osst << " (" << it->second.desc << ")" << endl;
	}
	
	MsgLog(name(), info, "------list of read out PVs in beginRun------\n" << osst.str() );
	//use the values
	MsgLog(name(), info, "run no " << runNumber);
	for (it = p_pvs.begin(); it != p_pvs.end(); it++){
		runinfo << it->first << " ";
	}
	runinfo << endl;
	
	fout << runinfo.str();
	
	
//	MsgLog(name(), info, "------quantities derived from selected PVs------" );
//	unsigned int detZ_no = 0;
//	unsigned int lambda_no = 11;	
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
	MsgLog(name(), debug,  "event()" );

	if (p_stopflag){
		stop();
		return;
	}

	//try to get the PV values
	map<string,pv>::iterator it;
	ostringstream osst;
	for (it = p_pvs.begin(); it != p_pvs.end(); it++){
		osst << setw(20) << it->first << " " << setw(26) << it->second.name; 
		try{
			it->second.value = env.epicsStore().value(it->second.name);
			it->second.valid = true;
			osst << " = " << setw(12) << it->second.value;
		}catch(...){
			it->second.valid = false;
			osst << setw(12) << " ---- ";
		}
		osst << " (" << it->second.desc << ")" << endl;
	}
	MsgLog(name(), info, "------list of read out PVs in event #" << p_count << "------\n" << osst.str() );

	//stop after a few events
	if (p_count >= 10){
		p_stopflag = true;
	}
		
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
