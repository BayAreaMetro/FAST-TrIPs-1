/*-------------------------------------------------------
FAST-TrIPs: Flexible Assignment and Simulation Tool for Transit and Intermodal Passengers
Copyright 2014 Alireza Khani and Mark Hickman
Licensed under the Apache License, Version 2.0
-------------------------------------------------------
Code primarily written by Alireza Khani
Under supervision of Mark Hickman

Contact:
    Alireza Khani:  akhani@utexas.edu or akhani@email.arizona.edu
    Mark Hickman:   m.hickman1@uq.edu.au
-------------------------------------------------------
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-------------------------------------------------------*/

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
using namespace std;
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class passenger{
protected:
	string				passengerString;
	string				passengerId;
	string				passengerOriginTAZ;
	string				passengerDestinationTAZ;
	int					passengerMode;
	int					passengerTimePeriod;
	int					passengerTourHalf;
	double				passengerPDT;
	double				passengerPAT;
	string				passengerTazTime;
	int                 passengerPathId;

	//Simulation
	string				assignedPath;
	double				startTime;
	double				endTime;
	int 				lengthOfPassengerTrip;
	vector<string>		boardingStops;
	vector<string>		trips;
	vector<string>		alightingStops;
	vector<double>		walkingTimes;

	int					passengerStatus;	//-1=not assigned, 0=assigned, 1=walking, 2=waiting, 3=on board, 4=missed, 5=arrived,
	int					passengerPathIndex;
	vector<double>		experiencedArrivalTimes;
	vector<double>		experiencedBoardingTimes;
	vector<double>		experiencedAlightingTimes;
	string				experiencedPath;
	double				experiencedCost;

    //path-based assignment
    map<string,int>						pathSet;
    map<string,int>::iterator			pathIter;
    map<string,double>					pathUtility;
	map<string,double>::iterator		pathIter2;
    map<string,int>                     pathCapacity;

    // lmz: this is just for ordering paths so we process them in the same order as fast-trips code
    typedef struct {
    	vector<int> stops;
    	vector<int> trips;
    } PathCompat;

    struct PathCompatCompare {
    	// less than
    	bool operator()(const PathCompat &path1, const PathCompat &path2) const {
    		if (path1.stops.size() < path2.stops.size()) { return true;  }
    		if (path1.stops.size() > path2.stops.size()) { return false; }
    		// if number of stops matches, check the stop ids and trip id
    		for (int ind=0; ind<path1.stops.size(); ++ind) {
    			if (path1.stops[ind] < path2.stops[ind]) { return true;  }
    			if (path1.stops[ind] > path2.stops[ind]) { return false; }
    			if (path1.trips[ind] < path2.trips[ind]) { return true;  }
    			if (path1.trips[ind] > path2.trips[ind]) { return false; }
    		}
    		return false;
    	}
    };

    // store it here. PathCompat -> path string
    map<PathCompat, string, struct passenger::PathCompatCompare> 			pathCompatMap;
    
public:
	passenger(){}
	~passenger(){}

	void			initializePassenger(string _passengerStr, int _pathId);
	string			getPassengerString();
	string			getPassengerId();
	string			getOriginTAZ();
	string			getDestinationTAZ();
	double			getPDT();
	double			getPAT();
	int				getTourHalf();
	int				getMode();
	string			getTazTime();
	int				getTimePeriod();

	//For Assignment
	void			setAssignedPath(string _tmpPath);
	string			getAssignedPath();
	void			setRandSeed();

	//For Simulation
	void			resetPathInfo();
	int				initializePath();
	int				getPathIndex();
	double			getStartTime();
	void			setPassengerStatus(int	_status);
	int				getPassengerStatus();
	void			increasePathIndex();
	void			setEndTime(double _endTime);
	string			getCurrentBoardingStopId();
	string			getCurrentTripId();
	string			getCurrentAlightingStopId();
	double			getCurrentWalkingTime();

	void			addArrivalTime(double _arrivalTime);
	void			addBoardingTime(double	_boardingTime);
	void			addAlightingTime(double _alightingTime);
	double			getArrivalTime();
	double			getBoardingTime();
	double			getAlightingTime();
	string			getExperiencedPath();

	//For Available Capacity Definition
	string			getLastTripId();
    string			getLastAlightingStop();

	void			calculateExperiencedCost();
	double			getExperiencedCost();

    //path-based assignment
    void            resetPaths();
    void            addPaths(string _tmpPath);
    void            analyzePaths(bool trace);
    string          assignPath(bool trace);
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////
map<string,passenger*>				passengerSet;
list<passenger*>					passengerList;
map<string,string>					tazTimeSet;
list<passenger*>					passengers2transfer;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
int			readPassengers(){
	string			tmpIn, buf, tmpPassengerId, tmpMode;
	vector<string>	tokens;
	passenger*		tmpPassengerPntr;

	ifstream inFile;
	inFile.open("ft_input_demand.dat");
	if (!inFile) {
		cerr << "Unable to open file ft_input_demand.dat";
		exit(1);
	}
	int path_id = 0; // for compat with fast-trips, unique per path (not nec the same as p)
	getline(inFile,tmpIn);
	while (!inFile.eof()){
		buf.clear();
		tokens.clear();
		getline(inFile,tmpIn);
		if(tmpIn=="")	continue;
		stringstream ss(tmpIn);
		while (ss >> buf){
			tokens.push_back(buf);
		}
		tmpPassengerId = "p";
		tmpPassengerId.append(tokens[0]);
		tmpPassengerPntr = NULL;
		tmpPassengerPntr = new passenger;
		passengerSet[tmpPassengerId] = tmpPassengerPntr;
		passengerList.push_back(tmpPassengerPntr);
		passengerSet[tmpPassengerId]->initializePassenger(tmpIn, path_id);
		path_id++;
	}
	inFile.close();
	return passengerSet.size();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int			readExistingPaths(){
	int				tmpNumPaths;
	string			tmpIn, buf, tmpPassengerId, tmpPath;
	vector<string>	tokens;

	ifstream inFile;
	inFile.open("ft_output_passengerPaths.dat");
	if (!inFile) {
		cerr << "Unable to open file ft_output_passengerPaths.dat";
		exit(1);
	}

	tmpNumPaths = 0;
	getline(inFile,tmpIn);
	while (!inFile.eof()){
		buf.clear();
		tokens.clear();
		getline(inFile,tmpIn);
		if(tmpIn=="")	continue;
		stringstream ss(tmpIn);
		while (ss >> buf){
			tokens.push_back(buf);
		}
		tmpPassengerId = "p";
		tmpPassengerId.append(tokens[0]);
		tmpPath = tokens[4] + "\t" + tokens[5] + "\t" + tokens[6] + "\t" +  tokens[7] + "\t" + tokens[8];
		passengerSet[tmpPassengerId]->setAssignedPath(tmpPath);
		tmpNumPaths++;
	}
	inFile.close();
	return tmpNumPaths;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void		passenger::initializePassenger(string _tmpIn, int _pathId){
	string				buf, tmpStr;
	vector<string>		tokens;
	char				chr[99];

	passengerString = _tmpIn;
	buf.clear();
	tokens.clear();
	stringstream	ss1(_tmpIn);
	while (ss1 >> buf){
		tokens.push_back(buf);
	}
	passengerOriginTAZ = "t" + tokens[1];
	passengerDestinationTAZ = "t" + tokens[2];
	passengerMode = atoi(tokens[3].c_str());
	passengerTimePeriod = atoi(tokens[4].c_str());
	passengerTourHalf = atoi(tokens[5].c_str());
	if(passengerTourHalf==1){
		passengerPAT = atof(tokens[6].c_str());
		if(passengerPAT<180){
			passengerPAT = passengerPAT + 1440;
		}
		passengerPDT = 0;
	}else{
		passengerPDT = atof(tokens[6].c_str());
		if(passengerPDT<180){
			passengerPDT = passengerPDT + 1440;
		}
		passengerPAT = 1800;
	}
	passengerId = "p";
	passengerId.append(tokens[0]);
	passengerPathId = _pathId;

	passengerStatus = -1;
	experiencedCost = 999999;
	startTime = -101;
}
string		passenger::getPassengerString(){
	return this->passengerString;
}
string		passenger::getPassengerId(){
	return this->passengerId;
}
string		passenger::getOriginTAZ(){
	return this->passengerOriginTAZ;
}
string		passenger::getDestinationTAZ(){
	return this->passengerDestinationTAZ;
}
double		passenger::getPDT(){
	return this->passengerPDT;
}
double		passenger::getPAT(){
	return this->passengerPAT;
}
int			passenger::getTourHalf(){
	return this->passengerTourHalf;
}
int			passenger::getMode(){
	return this->passengerMode;
}
void		passenger::setAssignedPath(string _tmpPath){
	assignedPath = _tmpPath;
}
string		passenger::getAssignedPath(){
	return this->assignedPath;
}
void		passenger::setRandSeed() {
	srand(this->passengerPathId);
}
int			passenger::getTimePeriod(){
	return this->passengerTimePeriod;
}
/////////////////////////////////////////////////////////////////////////////////////////////
void		passenger::resetPathInfo(){
	startTime = -101;
	endTime = -101;
	lengthOfPassengerTrip = 0;
	boardingStops.clear();
	trips.clear();
	alightingStops.clear();
	walkingTimes.clear();

	passengerStatus = -1;	//-1=not assigned, 0=assigned, 1=walking, 2=waiting, 3=on board, 4=missed, 5=arrived,
	passengerPathIndex = 0;

	experiencedArrivalTimes.clear();
	experiencedBoardingTimes.clear();
	experiencedAlightingTimes.clear();
	experiencedPath = "";
	//experiencedCost = 999999;
}
int			passenger::initializePath(){
	vector<string>		tokens;
	string				buf, tmpPathString, tmpBoardingStops, tmpTrips, tmpAlightingStops, tmpWalkingTimes;

	buf.clear();
	tokens.clear();
	tmpPathString = assignedPath;
	stringstream ss(tmpPathString);
	while (ss >> buf){
		tokens.push_back(buf);
	}
	startTime = atof(tokens[0].c_str());
	tmpBoardingStops = tokens[1];
	tmpTrips = tokens[2];
	tmpAlightingStops = tokens[3];
	tmpWalkingTimes = tokens[4];										//cout<<tmpWalkingTimes<<endl;

	//boardingStops.clear();
	stringstream ss1(tmpBoardingStops);
	while (getline(ss1, buf, ',')){
		boardingStops.push_back(buf);
	}
	//trips.clear();
	stringstream ss2(tmpTrips);
	while (getline(ss2, buf, ',')){
		trips.push_back(buf);
	}
	//alightingStops.clear();
	stringstream ss3(tmpAlightingStops);
	while (getline(ss3, buf, ',')){
		alightingStops.push_back(buf);
	}
	//walkingTimes.clear();
	stringstream ss4(tmpWalkingTimes);
	while (getline(ss4, buf, ',')){
		walkingTimes.push_back(atof(buf.c_str()));
	}

	lengthOfPassengerTrip = trips.size();
	passengerPathIndex = 0;

	//experiencedArrivalTimes.clear();
	//experiencedBoardingTimes.clear();
	//experiencedAlightingTimes.clear();
	//experiencedPath = "";
	//experiencedCost = 0;
	//endTime = -101;
	return lengthOfPassengerTrip;
}
int			passenger::getPathIndex(){
	return this->passengerPathIndex;
}
double		passenger::getStartTime(){
	return this->startTime;
}
void		passenger::setPassengerStatus(int _status){
	passengerStatus = _status;
}
int			passenger::getPassengerStatus(){
	return this->passengerStatus;
}
void		passenger::increasePathIndex(){
	passengerPathIndex = passengerPathIndex + 1;
}
void		passenger::setEndTime(double _endTime){
	endTime = _endTime;
}
string		passenger::getCurrentBoardingStopId(){
	if(passengerPathIndex<lengthOfPassengerTrip){
		return this->boardingStops[passengerPathIndex];
	}else{
		return "-101";
	}
}
string		passenger::getCurrentTripId(){
	if(passengerPathIndex<lengthOfPassengerTrip){
		return this->trips[passengerPathIndex];
	}else{
		return "-101";
	}
}
string		passenger::getCurrentAlightingStopId(){
	if(passengerPathIndex<lengthOfPassengerTrip){
		return this->alightingStops[passengerPathIndex];
	}else{
		return "-101";
	}
}
double		passenger::getCurrentWalkingTime(){
	return this->walkingTimes[passengerPathIndex];
}
void		passenger::addArrivalTime(double _arrivalTime){
	experiencedArrivalTimes.push_back(_arrivalTime);
}
void		passenger::addBoardingTime(double	_boardingTime){
	experiencedBoardingTimes.push_back(_boardingTime);
}
void		passenger::addAlightingTime(double _alightingTime){
	experiencedAlightingTimes.push_back(_alightingTime);
}
double		passenger::getArrivalTime(){
	return	this->experiencedArrivalTimes.back();
}
double		passenger::getBoardingTime(){
	return	this->experiencedBoardingTimes.back();
}
double		passenger::getAlightingTime(){
	if(passengerPathIndex == 0){
		return	this->startTime;
	}else{
		return	this->experiencedAlightingTimes.back();
	}
}
string		passenger::getExperiencedPath(){
	int		i;
	string	tmpExperiencedPath, tmpArrivalTimes, tmpBoardingTimes, tmpAlightingTimes;
	char	chr[99];
	for(i=0;i<experiencedArrivalTimes.size();i++){
		if(i!=0){
			tmpArrivalTimes.append(",");
			tmpBoardingTimes.append(",");
			tmpAlightingTimes.append(",");
		}
        sprintf(chr,"%d",int(100*experiencedArrivalTimes[i])/100);
        tmpArrivalTimes.append(string(chr));
		tmpArrivalTimes.append(".");
		if(int(100*experiencedArrivalTimes[i])%100<10)	tmpArrivalTimes.append("0");
        sprintf(chr,"%d",int(100*experiencedArrivalTimes[i])%100);
        tmpArrivalTimes.append(string(chr));

		sprintf(chr,"%d",int(100*experiencedBoardingTimes[i])/100);
        tmpBoardingTimes.append(string(chr));
        tmpBoardingTimes.append(".");
		if(int(100*experiencedBoardingTimes[i])%100<10)	tmpBoardingTimes.append("0");
        sprintf(chr,"%d",int(100*experiencedBoardingTimes[i])%100);
		tmpBoardingTimes.append(string(chr));

		sprintf(chr,"%d",int(100*experiencedAlightingTimes[i])/100);
        tmpAlightingTimes.append(string(chr));
        tmpAlightingTimes.append(".");
		if(int(100*experiencedAlightingTimes[i])%100<10)	tmpAlightingTimes.append("0");
		sprintf(chr,"%d",int(100*experiencedAlightingTimes[i])%100);
        tmpAlightingTimes.append(string(chr));
    }
	tmpExperiencedPath = passengerId.substr(1,99);
	tmpExperiencedPath.append("\t");
    sprintf(chr,"%d",passengerMode);
    tmpExperiencedPath.append(string(chr));
	tmpExperiencedPath.append("\t");
	tmpExperiencedPath.append(passengerOriginTAZ.substr(1,99));
	tmpExperiencedPath.append("\t");
	tmpExperiencedPath.append(passengerDestinationTAZ.substr(1,99));
	tmpExperiencedPath.append("\t");
    sprintf(chr,"%d",int(100*startTime)/100);
	tmpExperiencedPath.append(string(chr));
    tmpExperiencedPath.append(".");
	if(int(100*startTime)%100<10)	tmpExperiencedPath.append("0");
    sprintf(chr,"%d",int(100*startTime)%100);
	tmpExperiencedPath.append(string(chr));
    tmpExperiencedPath.append("\t");
    sprintf(chr,"%d",int(100*endTime)/100);
	tmpExperiencedPath.append(string(chr));
    tmpExperiencedPath.append(".");
	if(int(100*endTime)%100<10)	tmpExperiencedPath.append("0");
    sprintf(chr,"%d",int(100*endTime)%100);
	tmpExperiencedPath.append(string(chr));
    tmpExperiencedPath.append("\t");
	tmpExperiencedPath.append(tmpArrivalTimes);
	tmpExperiencedPath.append("\t");
	tmpExperiencedPath.append(tmpBoardingTimes);
	tmpExperiencedPath.append("\t");
	tmpExperiencedPath.append(tmpAlightingTimes);
	return tmpExperiencedPath;
}
//For Available Capacity Definition
string		passenger::getLastTripId(){
	if(passengerPathIndex>0){
		return	this->trips[passengerPathIndex];
	}else{
		return	"Access";
	}
}
string		passenger::getLastAlightingStop(){
	if(passengerPathIndex>0){
		return	this->alightingStops[passengerPathIndex];
	}else{
		return	"Origin";
	}
}
///////////////////////////////////////////////
void	passenger::calculateExperiencedCost(){
	double	tmpWaitingTime, tmpAccessWalkingTime, tmpEgressWalkingTime, tmpTransferWalkingTime, tmpInVehTime;
	int		tmpNumTransfers, i;

	if(passengerStatus==5){		//-1=not assigned, 0=assigned, 1=walking, 2=waiting, 3=on board, 4=missed, 5=arrived,
		tmpWaitingTime = 0;
		tmpAccessWalkingTime = 0;
		tmpEgressWalkingTime = 0;
		tmpTransferWalkingTime = 0;
		tmpInVehTime = 0;
		tmpNumTransfers = trips.size() - 1;
		for(i=0;i<trips.size();i++){
			tmpWaitingTime = tmpWaitingTime + experiencedBoardingTimes[i] - experiencedArrivalTimes[i];
			if(i>0){
				tmpTransferWalkingTime = tmpTransferWalkingTime + experiencedArrivalTimes[i] - experiencedAlightingTimes[i-1];
			}
			tmpInVehTime = tmpInVehTime + experiencedAlightingTimes[i] - experiencedBoardingTimes[i];
		}

		experiencedCost = inVehTimeEqv*tmpInVehTime + waitingEqv*tmpWaitingTime + transferWalkEqv*tmpTransferWalkingTime
						+ originWalkEqv*walkingTimes.front() + destinationWalkEqv*walkingTimes.back()
						+ transferPenalty*tmpNumTransfers + (60.0*fare/VOT)*(tmpNumTransfers+1);
	}else if(passengerStatus==4){
		experiencedCost = 999999;
	}else{
		//cout <<passengerId<<"\tSTATUS = "<<passengerStatus<<endl;
	}
}
double	passenger::getExperiencedCost(){
	return this->experiencedCost;
}
///////////////////////////////////////////////
void    passenger::resetPaths(){
    pathSet.clear();
}
void    passenger::addPaths(string _tmpPath){
    pathIter = pathSet.find(_tmpPath);
    if (pathIter==pathSet.end())
        pathSet[_tmpPath] = 1;
    else{
        pathSet[_tmpPath] = pathSet[_tmpPath] + 1;
    }
}
void    passenger::analyzePaths(bool trace){
	//For Choice Set Attributes
    int                         n;
	string					    buf, buf2;
	vector< vector<string> >	tokens;
	vector<string>			    tokens2;
	vector<string>			    tmpTrips, tmpBoardings, tmpAlightings, tmpWalkings;
	string					    tmpStartTime, tmpFromToAt;
	double					    NTR, IWT, IVT, TRT, ACD, EGD, TRD, ScDelay, FARE, tmpUtility;
	string  					routeComb, tripComb, transferComb; //transferComb for capacity constraints

	if (trace) {
		cout << "analyzePaths --- pathSet.size()=" << pathSet.size() << endl;
	}
    pathUtility.clear();
    pathCapacity.clear();
    pathCompatMap.clear();

    for(pathIter=pathSet.begin();pathIter!=pathSet.end();pathIter++){
        buf.clear();
        tokens.clear();
        stringstream ss((*pathIter).first);
        while (ss >> buf){
            stringstream ss2(buf);
            buf2.clear();
            tokens2.clear();
            while (getline(ss2, buf2, ',')){
                tokens2.push_back(buf2);
            }
            tokens.push_back(tokens2);
        }
        tmpStartTime = tokens[0][0];
        tmpTrips = tokens[2];
        tmpBoardings = tokens[1];
        tmpAlightings = tokens[3];
        tmpWalkings = tokens[4];

        // lmz - do this to not drop precision
        double start_time = tripSet[tmpTrips[0]]->getSchDepartureByStop(tmpBoardings[0]) - atof(tmpWalkings[0].c_str());
        if (trace) {
        	cout << "  pathIter=(\"" << pathIter->first << "\"," << pathIter->second << "), starttime=" << tmpStartTime << "; trips=[" ;
            for (vector<string>::const_iterator si=tmpTrips.begin(); si != tmpTrips.end(); ++si) { cout << "'" << *si << "',"; }
        	cout << "]; boardings=[";
        	for (vector<string>::const_iterator si=tmpBoardings.begin(); si != tmpBoardings.end(); ++si) { cout << "'" << *si << "',"; }
        	cout << "]; alightings=[";
        	for (vector<string>::const_iterator si=tmpAlightings.begin(); si != tmpAlightings.end(); ++si) { cout << "'" << *si << "',"; }
        	cout << "]; walkings=[";
        	for (vector<string>::const_iterator si=tmpWalkings.begin(); si != tmpWalkings.end(); ++si) { cout << "'" << *si << "',"; }
        	cout << "], start_time=" << start_time << endl;
        }
        // lmz - do this to process paths in an order consistent with fast-trips
        PathCompat path_compat;
        if (passengerTourHalf==1) {
        	// origin to destination
      		for (int ind=0; ind < tmpBoardings.size(); ++ind) {
      			// boarding stop
      			path_compat.stops.push_back(atoi(tmpBoardings[ind].substr(1,tmpBoardings[ind].length()-1).c_str()));
      			path_compat.trips.push_back(atoi(tmpTrips[ind].substr(1,tmpTrips[ind].length()-1).c_str()));
      			// alighting stop/transfer if differs from next boarding
      			if (ind + 1 < tmpBoardings.size() && tmpAlightings[ind] != tmpBoardings[ind+1]) {
	      			path_compat.stops.push_back(atoi(tmpAlightings[ind].substr(1,tmpAlightings[ind].length()-1).c_str()));
    	  			path_compat.trips.push_back(tmpTrips.size() > ind+1 ? -102 : -101); // transfer if more trips, or egress
    	  		}
      		}
      	} else {
			// destination to origin
			for (int ind = tmpAlightings.size()-1; ind >= 0; --ind) {
				// alighting stop
      			path_compat.stops.push_back(atoi(tmpAlightings[ind].substr(1,tmpAlightings[ind].length()-1).c_str()));
      			path_compat.trips.push_back(atoi(tmpTrips[ind].substr(1,tmpTrips[ind].length()-1).c_str()));
      			// boarding stop/transfer if differs from next alighting
      			if (ind - 1 >= 0 && tmpBoardings[ind] != tmpAlightings[ind-1]) {
	      			path_compat.stops.push_back(atoi(tmpBoardings[ind].substr(1,tmpBoardings[ind].length()-1).c_str()));
	      			path_compat.trips.push_back(tmpTrips.size() > ind+1 ? -102 : -100); // transfer if more trips, or access
	      		}
			}
      	}
      	pathCompatMap[path_compat] = pathIter->first;

        routeComb = "";
        tripComb = "";
        NTR = tmpTrips.size() - 1;
        // IWT = tripSet[tmpTrips[0]]->getSchDepartureByStop(tmpBoardings[0]) - atof(tmpWalkings[0].c_str()) - atof(tmpStartTime.c_str());
        // IWT = roundf(IWT * 100) / 100;
        IWT = 0; // lmz - is this false sometimes?
        IVT = 0;
        TRT = 0;
        ACD = atof(tmpWalkings[0].c_str());
        EGD = atof(tmpWalkings[tmpWalkings.size()-1].c_str());
        TRD = 0;
        if(passengerTourHalf==1){
            ScDelay = passengerPAT - passengerPAT; //should be fixed
        }else{
            // ScDelay = atof(tmpStartTime.c_str()) - passengerPDT;
            ScDelay = start_time - passengerPDT;
        }
        FARE = 0;
        pathCapacity[(*pathIter).first] = 1;
        
        for (n=0;n<tmpTrips.size();n++){
            if(routeComb==""){
                routeComb = tripSet[tmpTrips[n]]->getRouteId().substr(1,99);
            }else{
                routeComb = routeComb + "," + tripSet[tmpTrips[n]]->getRouteId().substr(1,99);
            }
            if(tripComb==""){
                tripComb = tmpTrips[n].substr(1,99);
            }else{
                tripComb = tripComb + "," + tmpTrips[n].substr(1,99);
            }
            IVT = IVT + tripSet[tmpTrips[n]]->getSchArrivalByStop(tmpAlightings[n]) - tripSet[tmpTrips[n]]->getSchDepartureByStop(tmpBoardings[n]);
            if(tripSet[tmpTrips[n]]->getSchArrivalByStop(tmpAlightings[n]) - tripSet[tmpTrips[n]]->getSchDepartureByStop(tmpBoardings[n]) < 0){
            	cout <<tripSet[tmpTrips[n]]->getSchArrivalByStop(tmpAlightings[n]) - tripSet[tmpTrips[n]]->getSchDepartureByStop(tmpBoardings[n])<<endl;
            }
            if(n!=0 && n!=tmpTrips.size()-1){
                TRT = TRT + tripSet[tmpTrips[n]]->getSchDepartureByStop(tmpBoardings[n]) - tripSet[tmpTrips[n-1]]->getSchArrivalByStop(tmpAlightings[n-1]) - atof(tmpWalkings[n].c_str());
                TRD = TRD + atof(tmpWalkings[n].c_str());
            }
            FARE = FARE + 1.0;
            if ((tripSet[tmpTrips[n]]->getRouteId()).length()>3 && (tripSet[tmpTrips[n]]->getRouteId()).substr(1,1)=="9") FARE = FARE + 1.50;
            if(n==0){
                tmpFromToAt = "Access," + tmpTrips[n] + "," + tmpBoardings[n];
                if(availableCapacity2.find(tmpFromToAt)!=availableCapacity2.end()){
                    if(availableCapacity2[tmpFromToAt]==0){
                        pathCapacity[(*pathIter).first] = 0;
                    }
                }
            }else{
                tmpFromToAt = tmpTrips[n-1] + "," + tmpAlightings[n-1] + "," + tmpTrips[n] + "," + tmpBoardings[n];
                if(availableCapacity2.find(tmpFromToAt)!=availableCapacity2.end()){
                    if(availableCapacity2[tmpFromToAt]==0){
                        pathCapacity[(*pathIter).first] = 0;
                    }
                }
            }
        }
        // IVT = roundf(IVT * 100) / 100;
        tmpUtility = inVehTimeEqv*IVT + waitingEqv*(IWT+TRT) + originWalkEqv*ACD + destinationWalkEqv*EGD + transferWalkEqv*TRD + transferPenalty*NTR + scheduleDelayEqv*ScDelay + 60*FARE/VOT;
        // tmpUtility = roundf(tmpUtility * 100) / 100;
        pathUtility[(*pathIter).first] = tmpUtility;
        if (trace) {
        	cout << "   IVT="<< IVT << "; IWT=" << IWT << "; TRT=" << TRT << "; ACD=" << ACD << "; EGD=" << EGD << "; TRD=" << TRD << "; NTR=" << NTR << "; scDelay=" << ScDelay << "; fare=" << FARE << endl;
			cout << "   pathUtility=" << tmpUtility << "; pathCapacity=" << pathCapacity[pathIter->first] << endl; }
        //cout	<<passengerId.substr(1,99)<<"\t"<<(*pathIter).second<<"\t"<<pathUtility[(*pathIter).first];
        //cout  <<"\t"<<NTR<<"\t"<<IWT<<"\t"<<IVT<<"\t"<<TRT<<"\t"<<ACD<<"\t"<<EGD<<"\t"<<TRD<<"\t"<<FARE<<endl;
    }
}
string  passenger::assignPath(bool trace){
	int				i, j, tmpAltProb, tmpMaxProb, tmpRandNum;
    double          tmpLogsum;
	vector<string>	tmpAlternatives;
	vector<int>		tmpAltProbabilities;

    //If there is no path in the path set, skip
    if(pathUtility.size()==0){
        if(pathSet.size()==0){
            //cout <<"NO ASSIGNED PATH"<<endl;
            return "-101";
        }else{
            cout <<"PATH SET != PATH UTILITY"<<endl;
        }
	}

    //Calculate the denominator of the logit model
	i = 0;
    tmpLogsum = 0;
	for(pathIter2=pathUtility.begin();pathIter2!=pathUtility.end();pathIter2++){
        if(pathCapacity[(*pathIter2).first]>0){
            i++;
            tmpLogsum = tmpLogsum + exp(-theta*(*pathIter2).second);
        }
    }
    if(i==0){
		//cout <<"i=0; ";
		return "-101";
    }
	if(tmpLogsum==0){
		cout <<"LOGSUM=0"<<endl;
		return "-101";
	}

    //calculate the probability of each alternative
	j=-1;
	tmpMaxProb = 0;
	// for(pathIter2=pathUtility.begin();pathIter2!=pathUtility.end();pathIter2++){
    for (map<PathCompat, string, struct passenger::PathCompatCompare>::iterator cpi = pathCompatMap.begin(); cpi != pathCompatMap.end(); ++cpi){
    	string path_string = cpi->second;

        if(pathCapacity[path_string]>0){
        	if (trace) {
        		cout << "  prob " << std::setfill(' ') << std::setw(8) << (exp(-theta*pathUtility[path_string]))/tmpLogsum;
        		cout << " -> " << std::setw(8) << int(RAND_MAX*(exp(-theta*pathUtility[path_string]))/tmpLogsum);
        		cout << " cost " << std::setw(8) << pathUtility[path_string];
        		cout << " ==== " << path_string << endl;
        	}
            tmpAltProb = int(RAND_MAX*(exp(-theta*pathUtility[path_string]))/tmpLogsum);
            if(tmpAltProb < 1){
                continue;
            }
            j++;
            if(j>0)	tmpAltProb = tmpAltProb + tmpAltProbabilities[j-1];

            tmpAlternatives.push_back(path_string);
            tmpAltProbabilities.push_back(tmpAltProb);
            tmpMaxProb = tmpAltProb;
        }
	}
	if(tmpMaxProb<1){
		cout <<"MAXPROB=0"<<endl;
		return "-101";
	}

	if (trace) {
		for (j=0;j<tmpAlternatives.size();j++) {
			cout << "  j=" << j << "; prob=" << tmpAltProbabilities[j] << "; tmpAlternatives=" << tmpAlternatives[j] << endl;
		}
	}

    //select an alternative
	tmpRandNum = rand();
    if (trace) { cout << "  tmpRandNum=" << tmpRandNum << " -> " << tmpRandNum%tmpMaxProb << endl; }
	tmpRandNum = tmpRandNum%tmpMaxProb;
	for(j=0;j<tmpAlternatives.size();j++){
		if(tmpRandNum <= tmpAltProbabilities[j]){
			assignedPath = tmpAlternatives[j];
            return tmpAlternatives[j];
		}
	}

    //If nothing returned, return error!
	cout <<"WHAT?"<<endl;
	return "-101";
}
