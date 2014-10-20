#include "LeachWaveApplLayer.h"

const simsignalwrap_t LeachWaveApplLayer::mobilityStateChangedSignal = simsignalwrap_t(MIXIM_SIGNAL_MOBILITY_CHANGE_NAME);

void LeachWaveApplLayer::initialize(int stage) {
    BaseApplLayer::initialize(stage);

    if (stage==0) {
        myMac = FindModule<WaveAppToMac1609_4Interface*>::findSubModule(getParentModule());
        assert(myMac);

        myId = getParentModule()->getIndex();

        pCH=par("Prob_CH").doubleValue();

        headerLength = par("headerLength").longValue();
        double maxOffset = par("maxOffset").doubleValue();
        sendBeacons = par("sendBeacons").boolValue();
        beaconLengthBits = par("beaconLengthBits").longValue();
        beaconPriority = par("beaconPriority").longValue();

        sendData = par("sendData").boolValue();
        dataLengthBits = par("dataLengthBits").longValue();
        dataOnSch = par("dataOnSch").boolValue();
        dataPriority = par("dataPriority").longValue();


        //invia messaggio a se stesso per il timer dei turni

        T_Turn = new cMessage("Timer Turns", TIMER_TURN);
        //simulate asynchronous channel access
        double offSet = dblrand() * (par("beaconInterval").doubleValue()/2);
        offSet = offSet + floor(offSet/0.050)*0.050;
        individualOffset = dblrand() * maxOffset;

        findHost()->subscribe(mobilityStateChangedSignal, this);

        scheduleAt(simTime() + offSet, T_Turn);

        //primo turno del nodo scelgo se CH
        if(chElection()){
            //invia messaggio broadcast IO SONO CH
            sendWSM(prepareWSM("CH_MESSAGE", beaconLengthBits, type_CCH, beaconPriority, 0, -1));
        }
    }
}

void LeachWaveApplLayer::newTurn(){
    nTurn++;
}
bool LeachWaveApplLayer::chElection(){
    if(nextCHTurn == 0){
        int P_fraz;
        P_fraz=1/pCH;
        if(uniform(0,1) < pCH/(1-pCH*(nTurn%P_fraz))){
            state = 'CH';
            nextCHTurn=9;
            return true;
        }
    }
    state = 'FN';
    nextCHTurn--;
    return false;
}

WaveShortMessage*  LeachWaveApplLayer::prepareWSM(std::string name, int lengthBits, t_channel channel, int priority, int rcvId, int serial) {
    WaveShortMessage* wsm =     new WaveShortMessage(name.c_str());
    wsm->addBitLength(headerLength);
    wsm->addBitLength(lengthBits);

    switch (channel) {
        case type_SCH: wsm->setChannelNumber(Channels::SCH1); break; //will be rewritten at Mac1609_4 to actual Service Channel. This is just so no controlInfo is needed
        case type_CCH: wsm->setChannelNumber(Channels::CCH); break;
    }
    wsm->setPsid(0);
    wsm->setPriority(priority);
    wsm->setWsmVersion(1);
    wsm->setTimestamp(simTime());
    wsm->setSenderAddress(myId);
    wsm->setRecipientAddress(rcvId);
    wsm->setSenderPos(curPosition);
    wsm->setSerial(serial);

    if (name == "CH_MESSAGE") {
        DBG << "Creating CH_MESSAGE with Priority " << priority << " at Applayer at " << wsm->getTimestamp() << std::endl;
        }

    if (name == "ASSOCIATION_REQUEST") {
        DBG << "Creating ASSOCIATION_REQUEST with Priority " << priority << " at Applayer at " << wsm->getTimestamp() << std::endl;
    }

    return wsm;
}

void LeachWaveApplLayer::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj) {
    Enter_Method_Silent();
    if (signalID == mobilityStateChangedSignal) {
        handlePositionUpdate(obj);
    }
}

void LeachWaveApplLayer::handlePositionUpdate(cObject* obj) {
    ChannelMobilityPtrType const mobility = check_and_cast<ChannelMobilityPtrType>(obj);
    curPosition = mobility->getCurrentPosition();
}

void LeachWaveApplLayer::handleLowerMsg(cMessage* msg) {
    WaveShortMessage* wsm = dynamic_cast<WaveShortMessage*>(msg);
    ASSERT(wsm);

    if (std::string(wsm->getName()) == "CH_MESSAGE") {
        if(state=='FN'){
            //divento suo ON e lo informo
            sendWSM( prepareWSM("ASSOCIATION_REQUEST", beaconLengthBits, type_CCH, beaconPriority, wsm->getSenderAddress(), -1) );
            state='ON';
            //aggiungo il mittente al mio CH
        }
    }
    if (std::string(wsm->getName()) == "ASSOCIATION_REQUEST") {
        sendWSM( prepareWSM("ASSOCIATION_RESPONSE", beaconLengthBits, type_CCH, beaconPriority, wsm->getSenderAddress(), -1) );
        //aggiungo il mittente alla liste dei miei ON
    }
}

void LeachWaveApplLayer::handleSelfMsg(cMessage* msg) {
    //L'unico messaggio che arriva da se stesso è il timer dei turni
    switch (msg->getKind()) {
            case TIMER_TURN: {
                scheduleAt(simTime() + par("beaconInterval").doubleValue(), T_Turn);
                newTurn();
                if(chElection()){
                    //invia messaggio broadcast IO SONO CH
                }
                break;
            }
            default: {
                if (msg)
                    DBG << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
                break;
            }
        }
}

void LeachWaveApplLayer::sendWSM(WaveShortMessage* wsm) {
    sendDown(wsm);
}

void LeachWaveApplLayer::finish() {
    if (T_Turn->isScheduled()) {
        cancelAndDelete(T_Turn);
    }
    else {
        delete T_Turn;
    }

    findHost()->unsubscribe(mobilityStateChangedSignal, this);

}

LeachWaveApplLayer::~LeachWaveApplLayer() {

}
