//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __ADDRESSING_TEST_SATGATEWAYPHY_H_
#define __ADDRESSING_TEST_SATGATEWAYPHY_H_

#include <omnetpp.h>
#include "satFrameFwd_m.h"
#include "satFrameRtn_m.h"
#include "CRDSA.h"
#include <math.h>
//using namespace omnetpp;

/**
 * TODO - Generated class
 */
class SatGatewayPhy : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

  private:
    std::vector<SatFrameRtn*> retrievePackets();

    int     remoteInGateId;
    cModule **satTerminalModule;
    double propagationDelay;
    double slotDuration;
    int slotsPerFrame;
    int numberOfReplicas;



    std::vector<SatFrameRtn*> mPacketBuffer;


    std::vector<SatFrameRtn*> mPacketBufferFrame;

    std::vector<SatFrameRtn*> retrievedPacketsAll;


//    std::vector<SatFrameRtn*> mPacketBufferAll;

    std::vector<SatFrameRtn*> mPacketBufferSlot;

    cMessage *dumFirst =nullptr;

    long long count = 0;


    int totalNumberOfPacketsSend;
    int totalNumberOfPacketsReceived;


    cMessage *lastPacket=nullptr;
    //delay
    simtime_t retrievePacketsTime;
    simtime_t delaySum=0;
    simtime_t delayAvg=0;

    //设置高斯噪声功率
    double noisyPower = 0.5;//单位为w

    //设置信号发送功率
    double transmitPower = 0.7;//单位为w

    //设置信号B
    double bandwidthChannel = 1;//单位为w

    //设置门限
    //设置C码率

    double C = 0.5;//单位为w


    //double thresholdSNR= pow(2,C/bandwidthChannel)-1;
    double thresholdSNRInner= 100.77;
    double thresholdSNROuter= 0.77;
    bool isFrameTriggered;
    bool isClear;
    CRDSA* mCRDSA;
    simsignal_t delaySignal;
};

#endif
