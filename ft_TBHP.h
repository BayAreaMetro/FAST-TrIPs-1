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

#include <time.h>
#include <stdlib.h>
#include <iomanip>      // std::setprecision
using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
int			forwardTBHP(string _origin, double _PDT, int _timeBuffer, bool trace){
	int						i, j, numIterations, permanentLabel, tmpTransferStop;
	int						tmpNumAccess, tmpNumTransfers, tmpSeqNum, tmpMaxSeq;
	double					tmpCurrentLabel, tmpEarliestArrival, tmpOldLabel, tmpNewLabel, tmpNewCost, tmpNonWalkLabel;
	double					tmpAccessTime, tmpTransferTime, tmpNewDeparture, tmpNewArrival, tmpInVehTime, tmpWaitingTime;
	string					buf, tmpStr, tmpQueuvalue, tmpCurrentStop, tmpNewStop, tmpAccessibleTrips, tmpTrip, tmpCurrentMode, tmpNewMode;
	char					chr[99];
	vector<string>			tokens;
	list<taz*>::iterator	tmpTazListIter;
	taz*					tmpTazPntr;
	list<stop*>::iterator	tmpStopListIter;
	stop*					tmpStopPntr;
	list<trip*>::iterator	tmpTripListIter;
	trip*					tmpTripPntr;
	priority_queue<string>	stopQueue;

    if (trace) { cout << "origin: " << _origin << ", preferred_time: " << _PDT << endl; }

	//Initialization
	for(tmpTazListIter=tazList.begin();tmpTazListIter!=tazList.end();tmpTazListIter++){
		tmpTazPntr = NULL;
		tmpTazPntr = *tmpTazListIter;
		tmpTazPntr->resetTazStrategy();
	}
	for(tmpStopListIter=stopList.begin();tmpStopListIter!=stopList.end();tmpStopListIter++){
		tmpStopPntr = NULL;
		tmpStopPntr = *tmpStopListIter;
		tmpStopPntr->resetStopStrategy();
	}
	for(tmpTripListIter=tripList.begin();tmpTripListIter!=tripList.end();tmpTripListIter++){
		tmpTripPntr = NULL;
		tmpTripPntr = *tmpTripListIter;
		tmpTripPntr->resetTripUsedBefore(0);
	}
	stopQueue.empty();

	//Access from Origin TAZ
	tmpTazPntr = NULL;
	tmpTazPntr = tazSet[_origin];
	tmpTazPntr->forwardStrategyUpdate(1, _PDT, "Start", 1);
	tmpNumAccess = tmpTazPntr->getNumStops();
	for(i=0;i<tmpNumAccess;i++){
		tmpNewStop = tmpTazPntr->getStop(i);
		tmpAccessTime = tmpTazPntr->getAccessTime(i);
		tmpNewCost = 1 + originWalkEqv * tmpAccessTime;
		tmpNewLabel = tmpNewCost;
		tmpNewArrival = _PDT + tmpAccessTime;
		stopSet[tmpNewStop]->forwardStrategyUpdate(tmpNewLabel, tmpNewArrival, "Access", _origin, tmpNewCost, -101);
        sprintf(chr,"%d",int(999999-tmpNewLabel*1000));
        tmpQueuvalue = string(chr);
        tmpStr.resize(6-tmpQueuvalue.length(),'0');
        if (trace) {
        	cout << "access stop = " << tmpNewStop
            	 << "  tmpNewCost = " << fixed << setprecision(4) << tmpNewCost
             	 << "  tmpNewArrival = " << setfill('0') << setw(2) << int(tmpNewArrival/60.0)
             	 << ":" << setfill('0') << setw(2) << int(tmpNewArrival) % 60
             	 << ":" << setfill('0') << setw(2) << int(60*(tmpNewArrival - int(tmpNewArrival))) << endl;
        }
		tmpQueuvalue = tmpStr + tmpQueuvalue + tmpNewStop;
		stopQueue.push(tmpQueuvalue);
    }

	//Labeling loop
	numIterations = 0;
	while(stopQueue.size()>0){
		tmpStr = stopQueue.top();
		stopQueue.pop();
		tmpCurrentStop = tmpStr.substr(6,99);
		tmpStopPntr = NULL;
		tmpStopPntr = stopSet[tmpCurrentStop];
		tmpCurrentLabel = tmpStopPntr->getStrategyLabel();
		tmpNonWalkLabel = tmpStopPntr->getForwardNonWalkLabel();
		tmpEarliestArrival = tmpStopPntr->getStrategyEarliestArrival();
		tmpCurrentMode = tmpStopPntr->getArrivalTripId(0);
		permanentLabel = tmpStopPntr->getStrategyPermanentLabel();
		tmpTransferStop = tmpStopPntr->getTransferStop();
		if(permanentLabel==1 || tmpTransferStop==0){
			continue;
		}else{
			tmpStopPntr->setStrategyPermanentLabel();
		}

		if (trace) {
        	cout << "numIterations = " << numIterations
        	     << "; stop  "<< tmpCurrentStop
        	     << "; label " << tmpCurrentLabel
        	     << "; nonwalk " << tmpNonWalkLabel
        	     << "; earlyarr " << tmpEarliestArrival
        	     << "; mode = " << tmpCurrentMode
        	     << "; tmpTransferStop = " << tmpTransferStop << endl;
        }

		//Update by Transfers
		if(tmpCurrentMode!="Access"){
			tmpNumTransfers = tmpStopPntr->getNumTransfers();
			for(i=0;i<tmpNumTransfers;i++){
				tmpNewStop = tmpStopPntr->getTransfer(i);
				tmpTransferTime = tmpStopPntr->getTransferTime(i);
				tmpNewCost = tmpNonWalkLabel + transferWalkEqv * tmpTransferTime;
				tmpOldLabel = stopSet[tmpNewStop]->getStrategyLabel();
				if(tmpOldLabel == 999999){
					tmpNewLabel = tmpNewCost;
				}else{
					tmpNewLabel = exp(-1.0*theta*tmpOldLabel)+exp(-1.0*theta*tmpNewCost);
					tmpNewLabel = max(0.01,-1.0/theta*log(tmpNewLabel));
				}
				if(tmpNewLabel < 0){
					cout <<"Error - Negative Label - 1"<<endl;
				}
				if (trace) {
					cout << "  transfer i=" << setw(2) << setfill(' ') << i
						 << "; stop " << setw(7) << setfill(' ') << tmpNewStop
						 << "; label " << fixed << setprecision(4) << tmpNewLabel
						 << "; xfertime " << tmpTransferTime
						 << "; arrival " << tmpEarliestArrival + tmpTransferTime
						 << "; cost " << fixed << setprecision(4) << tmpNewCost
					     << "; oldlabel " << tmpOldLabel
						 << endl;
				}
				if(tmpNewLabel < 999 && tmpNewLabel > 0){
					tmpNewArrival = tmpEarliestArrival + tmpTransferTime;
					stopSet[tmpNewStop]->forwardStrategyUpdate(tmpNewLabel, tmpNewArrival, "Transfer", tmpCurrentStop, tmpNewCost, -101);
                    sprintf(chr,"%d",int(999999-tmpNewLabel*1000));
                    tmpQueuvalue = string(chr);
                    tmpStr.resize(6-tmpQueuvalue.length(),'0');
                    tmpQueuvalue = tmpStr + tmpQueuvalue + tmpNewStop;
                    stopQueue.push(tmpQueuvalue);
				}
			}
		}

		//Update by Trips
		tmpAccessibleTrips = tmpStopPntr->getForwardAccessibleTrips(tmpEarliestArrival, _timeBuffer);
		buf.clear();
		tokens.clear();
		stringstream	ss(tmpAccessibleTrips);
		while (ss >> buf){
			tokens.push_back(buf);
		}
		for(i=0;i<tokens.size();i=i+2){
			tmpTrip = tokens[i];
			tmpSeqNum = atoi(tokens[i+1].c_str());
			tmpTripPntr = tripSet[tmpTrip];
			if (tmpTripPntr->getTripUsedBefore(0)==1){
				continue;
			}
			tmpMaxSeq = tmpTripPntr->getMaxSequence();
			for(j=tmpSeqNum+1;j<=tmpMaxSeq;j++){
			//for(j=max(1,tmpSeqNum-1);j<tmpSeqNum;j++){	//LB
				tmpNewStop = tmpTripPntr->getStop(j);
				tmpNewMode = stopSet[tmpNewStop]->getArrivalTripId(0);
				if(tmpNewMode=="Access"){
					continue;
				}
				tmpNewArrival = tmpTripPntr->getSchArrival(j);
				tmpNewDeparture = tmpTripPntr->getSchDeparture(tmpSeqNum);
				tmpInVehTime = tmpNewArrival - tmpNewDeparture;  
				tmpWaitingTime = tmpNewDeparture - tmpEarliestArrival;
                if(tmpCurrentMode=="Transfer" || tmpCurrentMode.substr(0,1)=="t"){
					tmpNewCost = tmpCurrentLabel + tmpInVehTime +       waitingEqv * tmpWaitingTime + 60.0*fare/VOT + transferPenalty;
				}else{
					tmpNewCost = tmpCurrentLabel + tmpInVehTime + scheduleDelayEqv * tmpWaitingTime + 60.0*fare/VOT;
				}
				/*if ((tmpTripPntr->getRouteId()).length()>3 && (tmpTripPntr->getRouteId()).substr(1,1)=="9"){
                    tmpNewCost = tmpNewCost + (60.0*1.50)/VOT;
                }*/
				tmpOldLabel = stopSet[tmpNewStop]->getStrategyLabel();
				if(tmpOldLabel == 999999){
					tmpNewLabel = tmpNewCost;
				}else{
					tmpNewLabel = exp(-1.0*theta*tmpOldLabel)+exp(-1.0*theta*tmpNewCost);
					tmpNewLabel = max(0.01,-1.0/theta*log(tmpNewLabel));
				}
				if(tmpNewLabel < 0){
					cout <<"Error - Negative Label - 2"<<endl;
				}
				if(tmpNewLabel < 999 && tmpNewLabel > 0){
					if (trace) {
						cout << "  trip     j=" << setw(2) << setfill(' ') << j
							 << "; stop " << setw(7) << setfill(' ') << tmpNewStop
							 << "; label " << fixed << setprecision(4) << tmpNewLabel
							 << "; arrival " << tmpNewArrival
						     << "; mode " << tmpNewMode
							 << "; trip " << tmpTrip
							 << "; cost " << fixed << setprecision(4) << tmpNewCost
							 << "; departure " << tmpNewDeparture
						 	 << endl;
					}
					stopSet[tmpNewStop]->forwardStrategyUpdate(tmpNewLabel, tmpNewArrival, tmpTrip, tmpCurrentStop, tmpNewCost, tmpNewDeparture);
                    sprintf(chr,"%d",int(999999-tmpNewLabel*1000));
                    tmpQueuvalue = string(chr);
                    tmpStr.resize(6-tmpQueuvalue.length(),'0');
                    tmpQueuvalue = tmpStr + tmpQueuvalue + tmpNewStop;
                    stopQueue.push(tmpQueuvalue);
				}
			}
			tmpTripPntr->setTripUsedBefore(0);
		}
		numIterations++;
	}

	//Connect to All Other TAZ Centroid
	for(tmpTazListIter=tazList.begin();tmpTazListIter!=tazList.end();tmpTazListIter++){
		tmpTazPntr = NULL;
		tmpTazPntr = *tmpTazListIter;
		tmpNumAccess = tmpTazPntr->getNumStops();
		for(i=0;i<tmpNumAccess;i++){
			tmpOldLabel = tmpTazPntr->getStrategyLabel();
			tmpNewStop = tmpTazPntr->getStop(i);
			tmpAccessTime = tmpTazPntr->getAccessTime(i);
			tmpNewArrival = stopSet[tmpNewStop]->getStrategyLatestArrival() - tmpAccessTime;
			tmpNonWalkLabel = stopSet[tmpNewStop]->getForwardNonWalkLabel();							
			tmpNewCost = tmpNonWalkLabel + originWalkEqv * tmpAccessTime;
			if(tmpOldLabel == 999999){
				tmpNewLabel = tmpNewCost;
			}else{
				tmpNewLabel = exp(-1.0*theta*tmpOldLabel)+exp(-1.0*theta*tmpNewCost);
				tmpNewLabel = max(0.01,-1.0/theta*log(tmpNewLabel));
			}
			if(tmpNewLabel < 0){
				cout <<"Error - Negative Label - 3"<<endl;
			}
			if(tmpNewLabel < 999 && tmpNewLabel > 0){
				if (trace) {
					cout << "  taz = " << tmpTazPntr->getTazId()
						 << "; stop " << setw(7)  << tmpNewStop
						 << "; label " << fixed << setprecision(4) << tmpNewLabel
						 << "; arrival " << tmpNewArrival
						 << "; cost " << fixed << setprecision(4) << tmpNewCost
						 << "; accesst " << tmpAccessTime
						 << "; nonwalk " << tmpNonWalkLabel
						 << "; oldlabel " << tmpOldLabel
						 << endl;
				}
				tmpTazPntr->forwardStrategyUpdate(tmpNewLabel, tmpNewArrival, tmpNewStop, tmpNewCost);
			}
		}
	}
	return numIterations;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int			backwardTBHP(string _destination, double _PAT, int _timeBuffer, bool trace){
	int						i, j, numIterations, permanentLabel, tmpTransferStop;;
	int						tmpNumAccess, tmpNumTransfers, tmpSeqNum;
	double					tmpCurrentLabel, tmpLatestDeparture, tmpEarliestDeparture, tmpOldLabel, tmpNewLabel, tmpNewCost, tmpNonWalkLabel;
	double					tmpAccessTime, tmpTransferTime, tmpNewDeparture, tmpNewArrival, tmpInVehTime, tmpWaitingTime;
	string					buf, tmpStr, tmpQueuvalue, tmpCurrentStop, tmpNewStop, tmpAccessibleTrips, tmpTrip, tmpCurrentMode, tmpNewMode;
	char					chr[99];
	vector<string>			tokens;
	list<taz*>::iterator	tmpTazListIter;
	taz*					tmpTazPntr;
	list<stop*>::iterator	tmpStopListIter;
	stop*					tmpStopPntr;
	list<trip*>::iterator	tmpTripListIter;
	trip*					tmpTripPntr;
	priority_queue<string>	stopQueue;

    if (trace) { cout << "destination: " << _destination << ", preferred_time: " << _PAT << endl; }

	//Initialization
	for(tmpTazListIter=tazList.begin();tmpTazListIter!=tazList.end();tmpTazListIter++){
		tmpTazPntr = NULL;
		tmpTazPntr = *tmpTazListIter;
		tmpTazPntr->resetTazStrategy();
	}
	for(tmpStopListIter=stopList.begin();tmpStopListIter!=stopList.end();tmpStopListIter++){
		tmpStopPntr = NULL;
		tmpStopPntr = *tmpStopListIter;
		tmpStopPntr->resetStopStrategy();
	}
	for(tmpTripListIter=tripList.begin();tmpTripListIter!=tripList.end();tmpTripListIter++){
		tmpTripPntr = NULL;
		tmpTripPntr = *tmpTripListIter;
		tmpTripPntr->resetTripUsedBefore(0);
	}
	stopQueue.empty();

	//Egress to Destination TAZ
	tmpTazPntr = NULL;
	tmpTazPntr = tazSet[_destination];
	tmpTazPntr->backwardStrategyUpdate(1, _PAT, "End", 1);
	tmpNumAccess = tmpTazPntr->getNumStops();
	for(i=0;i<tmpNumAccess;i++){
		tmpNewStop = tmpTazPntr->getStop(i);
		tmpAccessTime = tmpTazPntr->getAccessTime(i);
		tmpNewCost = 1 + destinationWalkEqv * tmpAccessTime;
		tmpNewLabel = tmpNewCost;
		tmpNewDeparture = _PAT - tmpAccessTime;
		stopSet[tmpNewStop]->backwardStrategyUpdate(tmpNewLabel, tmpNewDeparture, "Egress", _destination, tmpNewCost, -101);
		sprintf(chr,"%d",int(999999-tmpNewLabel*1000));
        tmpQueuvalue = string(chr);
        tmpStr.resize(6-tmpQueuvalue.length(),'0');
        if (trace) {
        	cout << "tmpStr=" << tmpStr 
            	 << "  tmpQueuevalue=" << tmpQueuvalue
             	 << "  tmpNewStop=" << tmpNewStop;
        }
		tmpQueuvalue = tmpStr + tmpQueuvalue + tmpNewStop;
		if (trace) { cout << " -> " << tmpQueuvalue << endl; }
		stopQueue.push(tmpQueuvalue);
	}

	//Labeling loop
	numIterations = 0;
	while(stopQueue.size()>0){
		tmpStr = stopQueue.top();
		stopQueue.pop();
		tmpCurrentStop = tmpStr.substr(6,99);
		tmpStopPntr = NULL;
		tmpStopPntr = stopSet[tmpCurrentStop];
		tmpCurrentLabel = tmpStopPntr->getStrategyLabel();
		tmpNonWalkLabel = tmpStopPntr->getBackwardNonWalkLabel();
		tmpLatestDeparture = tmpStopPntr->getStrategyLatestDeparture();
		tmpEarliestDeparture = tmpStopPntr->getStrategyEarliestDeparture();
		tmpCurrentMode = tmpStopPntr->getDepartureTripId(0);
		permanentLabel = tmpStopPntr->getStrategyPermanentLabel();
		tmpTransferStop = tmpStopPntr->getTransferStop();
		if(permanentLabel==1 || tmpTransferStop==0){
			// cout << "Continuing on stop " << tmpCurrentStop << "; permanentLabel=" << permanentLabel << "; tmpTransferStop=" << tmpTransferStop << endl;
			continue;
		}else{
			tmpStopPntr->setStrategyPermanentLabel();
		}

		if (trace) {
        	cout << "numIterations = " << numIterations
        	     << "; tmpCurrentStop = "<< tmpCurrentStop
        	     << "; tmpCurrentLabel = " << tmpCurrentLabel
        	     << "; tmpNonWalkLabel = " << tmpNonWalkLabel
        	     << "; tmpLatestDeparture = " << tmpLatestDeparture
        	     << "; tmpCurrentMode = " << tmpCurrentMode
        	     << "; xferstop " << tmpTransferStop << endl;
        }

		//Update by Transfers
		if(tmpCurrentMode!="Egress"){
			tmpNumTransfers = tmpStopPntr->getNumTransfers();
			for(i=0;i<tmpNumTransfers;i++){
				tmpNewStop = tmpStopPntr->getTransfer(i);
				tmpTransferTime = tmpStopPntr->getTransferTime(i);
				tmpNewCost = tmpNonWalkLabel + transferWalkEqv * tmpTransferTime;
				tmpOldLabel = stopSet[tmpNewStop]->getStrategyLabel();
				if(tmpOldLabel == 999999){
					tmpNewLabel = tmpNewCost;
				}else{
					tmpNewLabel = exp(-1.0*theta*tmpOldLabel)+exp(-1.0*theta*tmpNewCost);
					tmpNewLabel = max(0.01,-1.0/theta*log(tmpNewLabel));
				}
				if(tmpNewLabel < 0){
					cout <<"Error - Negative Label - 1"<<endl;
				}
				if (trace) {
					cout << "  transfer i=" << i
						 << "; tmpNewStop=" << tmpNewStop
					     << "; tmpOldLabel=" << tmpOldLabel
						 << "; tmpNewLabel=" << tmpNewLabel
						 << "; tmpTransferTime=" << tmpTransferTime
						 << "; tmpNewDeparture=" << tmpNewDeparture
						 << "; tmpNewCost=" << fixed << setprecision(4) << tmpNewCost
						 << endl;
				}
				if(tmpNewLabel < 999 && tmpNewLabel > 0){
					tmpNewDeparture = tmpLatestDeparture - tmpTransferTime;
					stopSet[tmpNewStop]->backwardStrategyUpdate(tmpNewLabel, tmpNewDeparture, "Transfer", tmpCurrentStop, tmpNewCost, -101);
                    sprintf(chr,"%d",int(999999-tmpNewLabel*1000));
                    tmpQueuvalue = string(chr);
                    tmpStr.resize(6-tmpQueuvalue.length(),'0');
                    tmpQueuvalue = tmpStr + tmpQueuvalue + tmpNewStop;
                    stopQueue.push(tmpQueuvalue);
				}
			}
		}

		//Update by Trips
		tmpAccessibleTrips = tmpStopPntr->getBackwardAccessibleTrips(tmpLatestDeparture, _timeBuffer);
		buf.clear();
		tokens.clear();
		stringstream	ss(tmpAccessibleTrips);
		while (ss >> buf){
			tokens.push_back(buf);
		}
		for(i=0;i<tokens.size();i=i+2){
			tmpTrip = tokens[i];
			tmpSeqNum = atoi(tokens[i+1].c_str());
			tmpTripPntr = tripSet[tmpTrip];
			if (tmpTripPntr->getTripUsedBefore(0)==1){
				continue;
			}
			for(j=1;j<tmpSeqNum;j++){
			//for(j=max(1,tmpSeqNum-1);j<tmpSeqNum;j++){	//LB
				tmpNewStop = tmpTripPntr->getStop(j);
				tmpNewMode = stopSet[tmpNewStop]->getDepartureTripId(0);
				if(tmpNewMode=="Egress"){
					continue;
				}
				tmpNewDeparture = tmpTripPntr->getSchDeparture(j);
				tmpNewArrival = tmpTripPntr->getSchArrival(tmpSeqNum);
				tmpInVehTime = tmpNewArrival - tmpNewDeparture;
                tmpWaitingTime = tmpLatestDeparture - tmpNewArrival;
				if(tmpCurrentMode=="Transfer" || tmpCurrentMode.substr(0,1)=="t"){
					tmpNewCost = tmpCurrentLabel + tmpInVehTime +       waitingEqv * tmpWaitingTime + 60.0*fare/VOT + transferPenalty;
				}else{
					tmpNewCost = tmpCurrentLabel + tmpInVehTime + scheduleDelayEqv * tmpWaitingTime + 60.0*fare/VOT;
				}
				/*if ((tmpTripPntr->getRouteId()).length()>3 && (tmpTripPntr->getRouteId()).substr(1,1)=="9"){
                    tmpNewCost = tmpNewCost + (60.0*1.50)/VOT;
                }*/
				tmpOldLabel = stopSet[tmpNewStop]->getStrategyLabel();
				if(tmpOldLabel == 999999){
					tmpNewLabel = tmpNewCost;
				}else{
					tmpNewLabel = exp(-1.0*theta*tmpOldLabel)+exp(-1.0*theta*tmpNewCost);
					tmpNewLabel = max(0.01,-1.0/theta*log(tmpNewLabel));
				}
				if(tmpNewLabel < 0){
					cout <<"Error - Negative Label - 2"<<endl;
				}
				if (trace) {
					cout << "  trip j=" << j
						 << "; tmpNewStop=" << tmpNewStop
						 << "; tmpNewLabel=" << tmpNewLabel
						 << "; tmpNewDeparture=" << tmpNewDeparture
						 << "; tmpTrip=" << tmpTrip
						 << "; tmpNewCost=" << tmpNewCost
						 << "; tmpNewArrival=" << tmpNewArrival
					     << "; tmpNewMode=" << tmpNewMode
					 	 << endl;
				}
				if(tmpNewLabel < 999 && tmpNewLabel > 0){
					stopSet[tmpNewStop]->backwardStrategyUpdate(tmpNewLabel, tmpNewDeparture, tmpTrip, tmpCurrentStop, tmpNewCost, tmpNewArrival);
                    sprintf(chr,"%d",int(999999-tmpNewLabel*1000));
                    tmpQueuvalue = string(chr);
                    tmpStr.resize(6-tmpQueuvalue.length(),'0');
                    tmpQueuvalue = tmpStr + tmpQueuvalue + tmpNewStop;
                    stopQueue.push(tmpQueuvalue);
				}
			}
			tmpTripPntr->setTripUsedBefore(0);
		}
		numIterations++;

	}

	//Connect to All Other TAZ Centroid
	for(tmpTazListIter=tazList.begin();tmpTazListIter!=tazList.end();tmpTazListIter++){
		tmpTazPntr = NULL;
		tmpTazPntr = *tmpTazListIter;
		tmpNumAccess = tmpTazPntr->getNumStops();
		for(i=0;i<tmpNumAccess;i++){
			tmpOldLabel = tmpTazPntr->getStrategyLabel();
			tmpNewStop = tmpTazPntr->getStop(i);
			tmpAccessTime = tmpTazPntr->getAccessTime(i);
			tmpNewDeparture = stopSet[tmpNewStop]->getStrategyEarliestDeparture() - tmpAccessTime;
			tmpNonWalkLabel = stopSet[tmpNewStop]->getBackwardNonWalkLabel();							
            tmpNewCost = tmpNonWalkLabel + originWalkEqv * tmpAccessTime;
			if(tmpOldLabel == 999999){
				tmpNewLabel = tmpNewCost;
			}else{
				tmpNewLabel = exp(-1.0*theta*tmpOldLabel)+exp(-1.0*theta*tmpNewCost);
				tmpNewLabel = max(0.01,-1.0/theta*log(tmpNewLabel));
			}
			if(tmpNewLabel < 0){
				cout <<"Error - Negative Label - 3"<<endl;
			}
			if (trace) {
				cout << "  taz =" << tmpTazPntr->getTazId()
					 << "; tmpNewStop=" << tmpNewStop
					 << "; tmpNewLabel=" << tmpNewLabel
					 << "; tmpNewDeparture=" << tmpNewDeparture
					 << "; tmpNewCost=" << tmpNewCost
					 << "; tmpAccessTime=" << tmpAccessTime
					 << "; tmpNonWalkLabel=" << tmpNonWalkLabel
					 << "; tmpOldLabel=" << tmpOldLabel
					 << endl;
			}
			if(tmpNewLabel < 999 && tmpNewLabel > 0){
				tmpTazPntr->backwardStrategyUpdate(tmpNewLabel, tmpNewDeparture, tmpNewStop, tmpNewCost);
			}
		}
	}
	return numIterations;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string		getForwardElementaryPath(string _destination, double _PAT, bool trace){
	int					tmpStrLen;
	string				tmpStr, tmpIn, tmpCurrentStop, tmpNewStop, tmpCurrentTrip, tmpAccessLink, tmpTransferLink, tmpLastTrip, tmpFirstTrip, tmpFirstStop;
	double				tmpDepartureTime, tmpStartTime, tmpEndTime;
	string				buf, tmpBoardingStops, tmpAlightingStops, tmpTrips, tmpWalkingTimes, tmpPath;	
	vector<string>		tokens;
	char				chr[99];

	if (trace) {
		cout << "TRACE" << endl;
	}

	tmpIn = tazSet[_destination]->getForwardAssignedAlternative(1800, trace);
	// cout << "tmpIn = [" << tmpIn << "]" << endl;
	if(tmpIn=="-101"){
		//cout <<"C1"<<endl;
		return "-101";
	}
	buf.clear();
	tokens.clear();
	stringstream	ss(tmpIn);
	while (ss >> buf){
		tokens.push_back(buf);
	}
	tmpCurrentStop = tokens[0];
	tmpEndTime = atof(tokens[1].c_str())/100;
	tmpAccessLink = _destination + "," + tmpCurrentStop;
    sprintf(chr,"%d",int(100*accessTimes[tmpAccessLink]+0.5));
    tmpIn = string(chr);
    tmpStrLen = tmpIn.length();
	tmpWalkingTimes = tmpIn.substr(0,max(0,tmpStrLen-2)) + ".";
	if(tmpStrLen<2)				tmpWalkingTimes = tmpWalkingTimes + "0";
	tmpWalkingTimes = tmpWalkingTimes + tmpIn.substr(max(0,tmpStrLen-2),2);
	tmpDepartureTime = tmpEndTime - accessTimes[tmpAccessLink];
	tmpLastTrip = "Egress";

	while(1){
		tmpIn = stopSet[tmpCurrentStop]->getForwardAssignedAlternative(tmpDepartureTime, tmpLastTrip, trace);
	    // cout << "tmpLastTrip = [" << tmpLastTrip
	    //      << "] departure = [" << tmpDepartureTime
	    //      << "] walk = [" << tmpWalkingTimes
	    //      << "] tmpFirstStop = [" << tmpFirstStop << "]" << endl;
     	// cout << "tmpIn = [" << tmpIn << "]" << endl;

		if(tmpIn=="-101"){
			//cout <<"C2"<<endl;
			return "-101";
		}
		buf.clear();
		tokens.clear();
		stringstream	ss(tmpIn);
		while (ss >> buf){
			tokens.push_back(buf);
		}
		tmpNewStop = tokens[0];
		tmpCurrentTrip = tokens[1];
		if(tmpNewStop.substr(0,1)=="s"){
			tmpFirstStop = tmpNewStop;
		}
		if(tmpCurrentTrip.substr(0,1)=="t"){
			tmpFirstTrip = tmpCurrentTrip;
		}
		if(tmpCurrentTrip=="Access"){
			tmpAccessLink = tmpNewStop + "," + tmpCurrentStop;		
            sprintf(chr,"%d",int(100*accessTimes[tmpAccessLink]+0.5));
            tmpIn = string(chr);
			tmpStrLen = tmpIn.length();
			tmpWalkingTimes = tmpIn.substr(max(0,tmpStrLen-2),2) + "," + tmpWalkingTimes;
			if(tmpStrLen<2)				tmpWalkingTimes = "0" + tmpWalkingTimes;
			tmpWalkingTimes = tmpIn.substr(0,max(0,tmpStrLen-2)) + "." + tmpWalkingTimes;
            tmpStartTime = tripSet[tmpFirstTrip]->getSchDepartureByStop(tmpFirstStop) - accessTimes[tmpAccessLink];
            sprintf(chr,"%d",int(100*tmpStartTime+0.5));
            tmpStr = string(chr);
			tmpStrLen = tmpStr.length();
			tmpPath = tmpStr.substr(0,max(0,tmpStrLen-2)) + ".";
			if(tmpStrLen<2)			tmpPath = tmpPath + "0";
			tmpPath = tmpPath + tmpStr.substr(max(0,tmpStrLen-2),2);
			tmpPath.append("\t");
			tmpPath.append(tmpBoardingStops);
			tmpPath.append("\t");
			tmpPath.append(tmpTrips);
			tmpPath.append("\t");
			tmpPath.append(tmpAlightingStops);
			tmpPath.append("\t");
			tmpPath.append(tmpWalkingTimes);
			return tmpPath;
		}else if(tmpCurrentTrip=="Transfer"){
			tmpTransferLink = tmpCurrentStop + "," + tmpNewStop;
            sprintf(chr,"%d",int(100*transferTimes[tmpTransferLink]+0.5));
            tmpIn = string(chr);
			tmpStrLen = tmpIn.length();
			tmpWalkingTimes = tmpIn.substr(max(0,tmpStrLen-2),2) + "," + tmpWalkingTimes;
			if(tmpStrLen<2)				tmpWalkingTimes = "0" + tmpWalkingTimes;
			tmpWalkingTimes = tmpIn.substr(0,max(0,tmpStrLen-2)) + "." + tmpWalkingTimes;
			tmpDepartureTime = tmpDepartureTime - transferTimes[tmpTransferLink];
			tmpLastTrip = tmpCurrentTrip;
		}else if(tmpCurrentTrip.substr(0,1)=="t"){
			if(tmpBoardingStops!="")	tmpBoardingStops = "," + tmpBoardingStops;
			tmpBoardingStops = tmpNewStop + tmpBoardingStops;
			if(tmpAlightingStops!="")	tmpAlightingStops = "," + tmpAlightingStops;
			tmpAlightingStops = tmpCurrentStop + tmpAlightingStops;
			if(tmpTrips!="")			tmpTrips = "," + tmpTrips;
			tmpTrips = tmpCurrentTrip + tmpTrips;
			if(tmpLastTrip.substr(0,1)=="t")		tmpWalkingTimes = "0," + tmpWalkingTimes;
			tmpDepartureTime = atof(tokens[3].c_str())/100;
			tmpLastTrip = tmpCurrentTrip;
		}else{
			cout <<"ERROR - TripId: "<<tmpCurrentTrip<<endl;
			return "-101";
		}
		tmpCurrentStop = tmpNewStop;
	}
	//cout <<"C3"<<endl;
	return "-101";
}///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string		getBackwardElementaryPath(string _origin, double _PDT, bool trace){
	int					i, tmpStrLen;
	string				tmpStr, tmpIn, tmpCurrentStop, tmpNewStop, tmpCurrentTrip, tmpAccessLink, tmpTransferLink, tmpLastTrip, tmpFirstTrip, tmpFirstStop;
	double				tmpArrivalTime, tmpStartTime, tmpDepartureTime;
	string				buf, tmpBoardingStops, tmpAlightingStops, tmpTrips, tmpWalkingTimes, tmpPath;	
	vector<string>		tokens;
	char				chr[99];

	tmpIn = tazSet[_origin]->getBackwardAssignedAlternative(0, trace);
	// cout << "tmpIn = [" << tmpIn << "]" << endl;
	if(tmpIn=="-101"){
		//cout <<"C1"<<endl;
		return "-101";
	}
	buf.clear();
	tokens.clear();
	stringstream	ss(tmpIn);
	while (ss >> buf){
		tokens.push_back(buf);
	}
	tmpCurrentStop = tokens[0];
	tmpStartTime = atof(tokens[1].c_str())/100;
    sprintf(chr,"%d",int(100*tmpStartTime));
    tmpStr = string(chr);
    tmpStrLen = tmpStr.length();
	tmpPath = tmpStr.substr(0,max(0,tmpStrLen-2)) + ".";
	if(tmpStrLen<2)			tmpPath = tmpPath + "0";
	tmpPath = tmpPath + tmpStr.substr(max(0,tmpStrLen-2),2);
    
    tmpAccessLink = _origin + "," + tmpCurrentStop;
	sprintf(chr,"%d",int(100*accessTimes[tmpAccessLink]+0.5));
    tmpIn = string(chr);
    tmpStrLen = tmpIn.length();
	tmpWalkingTimes = tmpWalkingTimes + tmpIn.substr(0,max(0,tmpStrLen-2)) + ".";
	if(tmpStrLen<2)			tmpWalkingTimes = tmpWalkingTimes + "0";
	tmpWalkingTimes = tmpWalkingTimes + tmpIn.substr(max(0,tmpStrLen-2),2);
	tmpArrivalTime = tmpStartTime + accessTimes[tmpAccessLink];
	tmpLastTrip = "Access";
	if(tmpCurrentStop.substr(0,1)=="s"){
		tmpFirstStop = tmpCurrentStop;
	}

	i = 0;
	while(1){
		tmpIn = stopSet[tmpCurrentStop]->getBackwardAssignedAlternative(tmpArrivalTime, tmpLastTrip, trace);
	    // cout << "tmpLastTrip = [" << tmpLastTrip
	    //      << "] tmpArrivalTime = [" << tmpArrivalTime
	    //      << "] tmpWalkingTimes = [" << tmpWalkingTimes
	    //      << "] tmpFirstStop = [" << tmpFirstStop << "]" << endl;
     	// cout << "tmpIn = [" << tmpIn << "]" << endl;

		if(tmpIn=="-101"){
			//cout <<"C2"<<endl;
			return "-101";
		}
		i++;
		buf.clear();
		tokens.clear();
		stringstream	ss(tmpIn);
		while (ss >> buf){
			tokens.push_back(buf);
		}
		tmpNewStop = tokens[0];
		tmpCurrentTrip = tokens[1];
		tmpDepartureTime = atof(tokens[2].c_str())/100;
		if(tmpCurrentTrip.substr(0,1)=="t"){
			tmpFirstTrip = tmpCurrentTrip;
		}
		if(i==1){
            tmpStartTime = tripSet[tmpFirstTrip]->getSchDepartureByStop(tmpFirstStop) - accessTimes[tmpAccessLink];
            sprintf(chr,"%d",int(100*tmpStartTime+0.5));
            tmpIn = string(chr);
            tmpStrLen = tmpIn.length();
			tmpPath = tmpIn.substr(0,max(0,tmpStrLen-2)) + ".";
            if(tmpStrLen<2)			tmpPath = tmpPath + "0";
			tmpPath = tmpPath + tmpIn.substr(max(0,tmpStrLen-2),2);
		}
		if(tmpCurrentTrip=="Egress"){
			tmpAccessLink = tmpNewStop + "," + tmpCurrentStop;		
			sprintf(chr,"%d",int(100*accessTimes[tmpAccessLink]+0.5));
            tmpIn = string(chr);
            tmpStrLen = tmpIn.length();
            tmpWalkingTimes = tmpWalkingTimes + "," + tmpIn.substr(0,max(0,tmpStrLen-2)) + ".";
			if(tmpStrLen<2)					tmpWalkingTimes = tmpWalkingTimes + "0";
			tmpWalkingTimes = tmpWalkingTimes + tmpIn.substr(max(0,tmpStrLen-2),2);

			tmpPath.append("\t");
			tmpPath.append(tmpBoardingStops);
			tmpPath.append("\t");
			tmpPath.append(tmpTrips);
			tmpPath.append("\t");
			tmpPath.append(tmpAlightingStops);
			tmpPath.append("\t");
			tmpPath.append(tmpWalkingTimes);
			return tmpPath;
		}else if(tmpCurrentTrip=="Transfer"){
			tmpTransferLink = tmpCurrentStop + "," + tmpNewStop;
			sprintf(chr,"%d",int(100*transferTimes[tmpTransferLink]+0.5));
            tmpIn = string(chr);
            tmpStrLen = tmpIn.length();
			tmpWalkingTimes = tmpWalkingTimes + "," + tmpIn.substr(0,max(0,tmpStrLen-2)) + ".";
			if(tmpStrLen<2)					tmpWalkingTimes = tmpWalkingTimes + "0";
			tmpWalkingTimes = tmpWalkingTimes + tmpIn.substr(max(0,tmpStrLen-2),2);
			tmpArrivalTime = tmpArrivalTime + transferTimes[tmpTransferLink];
			tmpLastTrip = tmpCurrentTrip;
		}else if(tmpCurrentTrip.substr(0,1)=="t"){
			if(tmpBoardingStops!="")	tmpBoardingStops.append(",");
			tmpBoardingStops.append(tmpCurrentStop);
			if(tmpAlightingStops!="")	tmpAlightingStops.append(",");
			tmpAlightingStops.append(tmpNewStop);
			if(tmpTrips!="")			tmpTrips.append(",");
			tmpTrips.append(tmpCurrentTrip);
			if(tmpLastTrip.substr(0,1)=="t")		tmpWalkingTimes.append(",0");
			tmpArrivalTime = atof(tokens[3].c_str())/100;
			tmpLastTrip = tmpCurrentTrip;
		}else{
			cout <<"ERROR - TripId: "<<tmpCurrentTrip<<endl;
			return "-101";
		}
		tmpCurrentStop = tmpNewStop;
	}
	//cout <<"C3"<<endl;
	return "-101";
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int		disaggregateStochasticAssignment(int _iter, int _timeBuff, int _numThreads){
	int									k, numThreads, tmpNumPassengers, tmpNumPaths;
	double								startTime, endTime, cpuTime;;
	list<passenger*>::iterator        	tmpPassengerListIter;

	numThreads = _numThreads;
	parallelizeStops(numThreads);
	parallelizeTazs(numThreads);
	parallelizeTrips(numThreads);

	cout <<"**************************** GENERATING PATHS ****************************"<<endl;
	tmpNumPassengers = passengerSet.size();
	tmpNumPaths = 0;
    startTime = clock()*1.0/CLOCKS_PER_SEC;
	for(tmpPassengerListIter=passengerList.begin();tmpPassengerListIter!=passengerList.end();tmpPassengerListIter++){
		int					                threadId, tmpNumIterations, tmpTourHalf, tmpStatus, m;
		string				                tmpPassengerId, tmpOriginTaz, tmpDestinationTaz, tmpPath;
		double				                tmpPDT, tmpPAT;
		passenger*			                passengerPntr;

		passengerPntr = NULL;
		passengerPntr = *tmpPassengerListIter;
		tmpPassengerId = passengerPntr->getPassengerId();

		tmpOriginTaz =passengerPntr->getOriginTAZ();
		tmpDestinationTaz = passengerPntr->getDestinationTAZ();
		if(tazSet.find(tmpOriginTaz)==tazSet.end() || tazSet.find(tmpDestinationTaz)==tazSet.end())	continue;
		if(tmpOriginTaz==tmpDestinationTaz)	continue;

		tmpStatus = passengerPntr->getPassengerStatus();
        if(_iter>1){
            if(tmpStatus==5){
                tmpNumPaths++;
                continue;
            }else{
                passengerPntr->setAssignedPath("");
                passengerPntr->setPassengerStatus(-1);
            }
        }
		tmpPDT = passengerPntr->getPDT();
		tmpPAT = passengerPntr->getPAT();
		tmpTourHalf = passengerPntr->getTourHalf();
		if(tmpTourHalf==1){
			tmpNumIterations = backwardTBHP(tmpDestinationTaz, tmpPAT, _timeBuff, tracePassengerId == tmpPassengerId);
			passengerPntr->setRandSeed();
			m = 0;
			while(1){
				tmpPath = getBackwardElementaryPath(tmpOriginTaz, tmpPDT, tracePassengerId == tmpPassengerId);
				m++;
				if(tmpPath!="-101" || m>1000){
					break;
				}
			}
		}else if(tmpTourHalf==2){
			tmpNumIterations = forwardTBHP(tmpOriginTaz, tmpPDT, _timeBuff, tracePassengerId == tmpPassengerId);
			passengerPntr->setRandSeed();
			m = 0;
			while(1){
				tmpPath = getForwardElementaryPath(tmpDestinationTaz, tmpPAT, tracePassengerId == tmpPassengerId);
				m++;
				if(tmpPath!="-101" || m>1000){
					break;
				}
			}
		}
		cout << "passenger " << tmpPassengerId << "; numIter " << tmpNumIterations << "; PDT " << tmpPDT << "; path = [" << tmpPath << "]" << endl;
		if(tmpPath!="-101"){
			passengerPntr->setAssignedPath(tmpPath);
			tmpNumPaths++;
		}
        if(k%max(min(tmpNumPassengers/10,1000),10)==0){
            endTime = clock()*1.0/CLOCKS_PER_SEC;
            cpuTime = round(100 * (endTime - startTime))/100.0;
			cout <<k<<" ( "<<tmpNumPaths<<" )\t/\t"<<tmpNumPassengers<<"\tpassengers assigned;\ttime elapsed:\t"<<cpuTime<<"\tseconds"<<endl;
		}
	}
    endTime = clock()*1.0/CLOCKS_PER_SEC;
    cpuTime = round(100 * (endTime - startTime))/100.0;
    cout <<k<<"\t/\t"<<tmpNumPassengers<<"\tpassengers assigned;\ttime elapsed:\t"<<cpuTime<<"\tseconds"<<endl;
	return tmpNumPaths;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int		pathBasedStochasticAssignment(int _iter, int _timeBuff, int _printPassengersFlag, int _numThreads){
	int									k, numThreads, tmpNumPassengers, tmpNumPaths;
	double								startTime, endTime, cpuTime;
	list<passenger*>::iterator        	tmpPassengerListIter;

    map<string,int>						tmpPathSet;
	map<string,int>::iterator			tmpPathSetIter;

	numThreads = _numThreads;
	parallelizeStops(numThreads);
	parallelizeTazs(numThreads);
	parallelizeTrips(numThreads);

    ofstream	outFile;
    if(_printPassengersFlag==1){
        if(_iter==1){
            outFile.open("ft_output_choiceSet.dat");
            outFile <<"passengerId\torigin\tdestination\tnumOfOccur\tpath"<<endl;
        }else{
            outFile.open("ft_output_choiceSet.dat",ofstream::app);
            outFile <<"ITERATION "<<_iter<<endl;
        }
    }

    cout <<"**************************** GENERATING PATHS ****************************"<<endl;
	startTime = clock()*1.0/CLOCKS_PER_SEC;
	tmpNumPassengers = passengerSet.size();
	tmpNumPaths = 0;
	// lmz: process all passengers without assuming passenger Id is unique
	for(tmpPassengerListIter=passengerList.begin();tmpPassengerListIter!=passengerList.end();tmpPassengerListIter++){
		int									threadId, tmpNumIterations, tmpTourHalf, tmpStatus, n;
		string								tmpPassengerId, tmpOriginTaz, tmpDestinationTaz, tmpPath;
		double								tmpPDT, tmpPAT;
		passenger*							passengerPntr;

		threadId = 0;

		passengerPntr = *tmpPassengerListIter;
		tmpPassengerId = passengerPntr->getPassengerId();

		tmpOriginTaz =passengerPntr->getOriginTAZ();
		tmpDestinationTaz = passengerPntr->getDestinationTAZ();
		if(tazSet.find(tmpOriginTaz)==tazSet.end() || tazSet.find(tmpDestinationTaz)==tazSet.end())	continue;
		if(tmpOriginTaz==tmpDestinationTaz)	continue;

		tmpStatus = passengerPntr->getPassengerStatus();
        if(_iter>1){
            if(tmpStatus==5){
                tmpNumPaths++;
                continue;
            }else if(tmpStatus==4){
                passengerPntr->setAssignedPath("");
                passengerPntr->analyzePaths(tracePassengerId == tmpPassengerId);
                tmpPath = passengerPntr->assignPath(tracePassengerId == tmpPassengerId);
                if(tmpPath!="-101"){
                    passengerPntr->setAssignedPath(tmpPath);
                    tmpNumPaths++;
                }
                continue;
            }else{
                passengerPntr->resetPaths();
                passengerPntr->setAssignedPath("");
                passengerPntr->setPassengerStatus(-1);
                continue;
            }
        }
		tmpPDT = passengerPntr->getPDT();
		tmpPAT = passengerPntr->getPAT();
		tmpTourHalf = passengerPntr->getTourHalf();
		tmpPathSet.clear();
        if (tmpTourHalf==1){
            tmpNumIterations = backwardTBHP(tmpDestinationTaz, tmpPAT, _timeBuff, tracePassengerId == tmpPassengerId);
   			passengerPntr->setRandSeed();
            for (n=1;n<=1000;n++){
                tmpPath = getBackwardElementaryPath(tmpOriginTaz, tmpPDT, tracePassengerId == tmpPassengerId);
                if (tracePassengerId == tmpPassengerId) {
                	cout << "Found path " << n << " " << tmpPath << endl;
                }
                if(tmpPath!="-101"){
                    tmpPathSetIter = tmpPathSet.find(tmpPath);
                    if (tmpPathSetIter==tmpPathSet.end())
                        tmpPathSet[tmpPath] = 1;
                    else{
                        tmpPathSet[tmpPath] = tmpPathSet[tmpPath] + 1;
                    }
                    passengerPntr->addPaths(tmpPath);
                }
            }
		}else{
            tmpNumIterations = forwardTBHP(tmpOriginTaz, tmpPDT, _timeBuff, tracePassengerId == tmpPassengerId);
   			passengerPntr->setRandSeed();
            for (n=1;n<=1000;n++){
                tmpPath = getForwardElementaryPath(tmpDestinationTaz, tmpPAT, (tracePassengerId == tmpPassengerId) && (n==465));
                if (tracePassengerId == tmpPassengerId) {
                	cout << "Found path " << n << " " << tmpPath << endl;
                }
                if(tmpPath!="-101"){
                    tmpPathSetIter = tmpPathSet.find(tmpPath);
                    if (tmpPathSetIter==tmpPathSet.end())
                        tmpPathSet[tmpPath] = 1;
                    else{
                        tmpPathSet[tmpPath] = tmpPathSet[tmpPath] + 1;
                    }
                    passengerPntr->addPaths(tmpPath);
                }
            }
		}
        if(_printPassengersFlag==1){
            for(tmpPathSetIter=tmpPathSet.begin();tmpPathSetIter!=tmpPathSet.end();tmpPathSetIter++){
                outFile <<tmpPassengerId.substr(1,99)<<"\t"<<tmpOriginTaz.substr(1,99)<<"\t"<<tmpDestinationTaz.substr(1,99)<<"\t"<<(*tmpPathSetIter).second<<"\t"<<(*tmpPathSetIter).first<<endl;
            }
        }

        passengerPntr->analyzePaths(tracePassengerId == tmpPassengerId);
        tmpPath = passengerPntr->assignPath(tracePassengerId == tmpPassengerId);
		if(tmpPath!="-101"){
			passengerPntr->setAssignedPath(tmpPath);
			tmpNumPaths++;
		}
		cout << "passenger " << tmpPassengerId << "; numIter " << tmpNumIterations << "; PAT/PDT " << tmpPAT << "/" << tmpPDT << "; path = [" << tmpPath << "]" << endl;

        if(k%max(min(tmpNumPassengers/10,1000),10)==0){
            endTime = clock()*1.0/CLOCKS_PER_SEC;
            cpuTime = round(100 * (endTime - startTime))/100.0;
			cout <<k<<" ( "<<tmpNumPaths<<" )\t/\t"<<tmpNumPassengers<<"\tpassengers assigned;\ttime elapsed:\t"<<cpuTime<<"\tseconds"<<endl;
		}
	}
    endTime = clock()*1.0/CLOCKS_PER_SEC;
    cpuTime = round(100 * (endTime - startTime))/100.0;
    cout <<k<<"\t/\t"<<tmpNumPassengers<<"\tpassengers assigned;\ttime elapsed:\t"<<cpuTime<<"\tseconds"<<endl;
    if(_printPassengersFlag==1){
       outFile.close();
    }
	return tmpNumPaths;
}
