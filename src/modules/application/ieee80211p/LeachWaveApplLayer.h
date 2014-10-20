#ifndef LEACHWAVEAPPLLAYER_H_
#define LEACHWAVEAPPLLAYER_H_


#include <map>
#include <BaseApplLayer.h>
#include <Consts80211p.h>
#include <WaveShortMessage_m.h>
#include "base/connectionManager/ChannelAccess.h"
#include <WaveAppToMac1609_4Interface.h>

#ifndef DBG
#define DBG EV
#endif

class LeachWaveApplLayer : public BaseApplLayer {

    public:
       ~LeachWaveApplLayer();
       virtual void initialize(int stage);
       virtual void finish();

       virtual  void receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj);

       enum WaveApplMessageKinds {
           SERVICE_PROVIDER = LAST_BASE_APPL_MESSAGE_KIND,
           TIMER_TURN,
           SEND_CH_BEACON,
           ASSOCIATION_REQUEST,
       };

    protected:

       static const simsignalwrap_t mobilityStateChangedSignal;
       /** @brief handle messages from below */
       virtual void handleLowerMsg(cMessage* msg);
       /** @brief handle self messages */
       virtual void handleSelfMsg(cMessage* msg);

       virtual WaveShortMessage* prepareWSM(std::string name, int dataLengthBits, t_channel channel, int priority, int rcvId, int serial=0);
       virtual void sendWSM(WaveShortMessage* wsm);
       virtual void onBeacon(WaveShortMessage* wsm) = 0;
       virtual void onData(WaveShortMessage* wsm) = 0;

       virtual void handlePositionUpdate(cObject* obj);

       virtual bool chElection();
       virtual void newTurn();

    protected:

       int beaconLengthBits;
       int beaconPriority;
       bool sendData;
       bool sendBeacons;
       simtime_t individualOffset;
       int dataLengthBits;
       bool dataOnSch;
       int dataPriority;
       Coord curPosition;
       int mySCH;
       int myId;

       char state; // CH (Cluster Head Node), ON (Ordinary Node), FN (Free Node)
       int nTurn=0;
       int nextCHTurn=0;
       double pCH;

       cMessage* T_Turn;
       cMessage* CH_Message;
       cMessage* associationRequest;

       WaveAppToMac1609_4Interface* myMac;

};





#endif
