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

#include "satGatewayPhy.h"

#include <fstream>
#include <algorithm>
#include   <vector>
using   namespace   std;
Define_Module(SatGatewayPhy);

void SatGatewayPhy::initialize()
{
    // Determine Id of remoteIn gate
    remoteInGateId = gate("remoteIn")->getId();

    // Get network parameter
    int nb_sat_terminals = getModuleByPath("Ipnet")->par("numSatTerminals");

    // Determine satellite terminal module
    satTerminalModule = new cModule*[nb_sat_terminals];
    for (int i=0; i<nb_sat_terminals; i++)
    {
        char module_name[25];
        sprintf(module_name, "Ipnet.satTerminal[%d]", i);
        satTerminalModule[i] = getModuleByPath(module_name);
    }

    totalNumberOfPacketsSend = 0;
    totalNumberOfPacketsReceived = 0;
    delaySignal = registerSignal("delay");

    propagationDelay = par("propagationDelay");
    slotDuration = par("slotDuration");
    slotsPerFrame = par("slotsPerFrame");
    numberOfReplicas = par("numberOfReplicas");

    //mPacketBufferAll.reserve(100000);

    isFrameTriggered = false;
    isClear=false;
    mCRDSA = new CRDSA(numberOfReplicas, slotsPerFrame);
}


void SatGatewayPhy::handleMessage(cMessage *msg)
{

    std::ofstream outputFile;
    outputFile.open("SimulationResults", std::ios::out | std::ios::app);
    // Get incoming packet and forward
    cPacket* incomingFrame = (cPacket*) msg;

    SatFrameRtn* msgtemp = (SatFrameRtn*)incomingFrame;
    //std::cout << "消息 = " << msg->getName() << std::endl;


    if(msg->isSelfMessage())
    {
        if(strcmp(msg->getName(),"frameTrigger")==0){

            //std::cout << "前调用mPacketBuffer = " << mPacketBuffer.size() << std::endl;
            //std::cout << "前调用mPacketBufferSlot = " << mPacketBufferSlot.size() << std::endl;

            std::vector<SatFrameRtn*> retrievedPackets = retrievePackets();
            mPacketBufferSlot.clear();

            //std::cout << "后Buffer = " << mPacketBuffer.size() << std::endl;
            //std::cout << "后BufferSlot = " << mPacketBufferSlot.size() << std::endl;
            retrievePacketsTime = simTime();
            while(retrievedPackets.size() > 0)
            {
                delaySum = delaySum + retrievePacketsTime - retrievedPackets[0]->getSendingTime();
                send(retrievedPackets[0]->decapsulate(), "localOut");
                delete retrievedPackets[0];
                retrievedPackets.erase(retrievedPackets.begin());
                totalNumberOfPacketsReceived++;
            }
            isFrameTriggered = false;
            delete msg;
            //outputFile << "/" <<  std::endl;
        }

        if(strcmp(msg->getName(),"mPacketBufferAllClear")==0){
            mPacketBuffer.clear();
            retrievedPacketsAll.clear();
            isClear = false;

            //outputFile << "%" <<  std::endl;
        }
    }
    // Return link
    else if(incomingFrame->getArrivalGateId() == remoteInGateId)
    {

        simtime_t creationFrame = incomingFrame->getArrivalTime(); // TODO: refactor

        if(!isFrameTriggered)
        {
            scheduleAt(creationFrame+slotDuration*1, new cMessage("frameTrigger"));
            isFrameTriggered = true;
        }

        int clear = incomingFrame->getCreationTime().dbl()/(slotDuration*slotsPerFrame)+0.0001; // TODO: refactor
        if(!isClear)
        {
            scheduleAt((clear+1)*slotDuration*slotsPerFrame+propagationDelay-0.0001, new cMessage("mPacketBufferAllClear"));
            isClear = true;
        }



        if(retrievedPacketsAll.size()==0){
             mPacketBuffer.push_back((SatFrameRtn*)incomingFrame);
             mPacketBufferSlot.push_back((SatFrameRtn*)incomingFrame);
             //outputFile << msgtemp->getSrcAddress()<<  msgtemp->getRandomSeed()<<  std::endl;
         }else{
             bool isExisted = false;

             for(unsigned int i=0; i<retrievedPacketsAll.size();i++){

                 int currentSrcAddressTemp = retrievedPacketsAll[i]->getSrcAddress();
                 int currentRandomSeedTemp = retrievedPacketsAll[i]->getRandomSeed();
                 if(currentSrcAddressTemp == msgtemp->getSrcAddress() && currentRandomSeedTemp == msgtemp->getRandomSeed()){
                     isExisted=true;
                     break;
                 }
             }

             if(!isExisted){
                 mPacketBuffer.push_back((SatFrameRtn*)incomingFrame);
                 mPacketBufferSlot.push_back((SatFrameRtn*)incomingFrame);
                 //outputFile << msgtemp->getSrcAddress()<<  msgtemp->getRandomSeed()<<  std::endl;

             }
         }

        totalNumberOfPacketsSend++;
    }
    else
    {
        // Get incoming L2 Frame and determine destination
        satFrameFwd *incomingL2Frame = (satFrameFwd *) incomingFrame;
        sendDirect(incomingL2Frame->decapsulate(), propagationDelay, 0, satTerminalModule[incomingL2Frame->getDestL2Address()], "satchannelforward");
        delete incomingL2Frame;
    }
    outputFile.close();
}


std::vector<SatFrameRtn*> SatGatewayPhy::retrievePackets()
{
    std::ofstream outputFile;
    outputFile.open("SimulationResults", std::ios::out | std::ios::app);

    std::vector<SatFrameRtn*> retrievedPackets;

    std::vector<SatFrameRtn*> retrievedTemp1;
    std::vector<SatFrameRtn*> retrievedTemp2;

    //时隙内译码
    //求总功率
    double noisyTotal=0;
    for(unsigned int i=0; i < mPacketBufferSlot.size(); i++)
    {

        noisyTotal=noisyTotal+ mPacketBufferSlot[i]->getChannelRatio()*transmitPower;
        //std::cout << "mPacketBufferSlot = " <<  mPacketBufferSlot[i]->getSrcAddress()<<"-"<<mPacketBufferSlot[i]->getRandomSeed() << std::endl;
    }

    //排序
    for(int i = 0;i<mPacketBufferSlot.size();i++)
    {
        for(int j = i+1;j<mPacketBufferSlot.size()-1;j++)
        {
            if(mPacketBufferSlot[i]->getChannelRatio() < mPacketBufferSlot[j]->getChannelRatio())
            {
                std::swap( mPacketBufferSlot[i],mPacketBufferSlot[j]);

            }
        }
    }

    //该时隙内的每个分组进行功率计算
    for(unsigned int i=0; i < mPacketBufferSlot.size(); i++)
    {
        //计算SNR
        double packetPower = mPacketBufferSlot[i]->getChannelRatio()*transmitPower;
        double SNRPacket=(packetPower)/(noisyPower+noisyTotal-packetPower);

        std::cout << "SNRPacket = " <<SNRPacket << std::endl;
        //判决门限
        if(SNRPacket>=thresholdSNRInner){
            //std::cout << "SNRPacket = " <<1 << std::endl;
           SatFrameRtn* p00= new SatFrameRtn;
            *p00 = *mPacketBufferSlot[i];
            retrievedTemp1.push_back(p00);
            noisyTotal=noisyTotal-packetPower;

        }else{
            //std::cout << "SNRPacket = " <<0 << std::endl;
        }

        std::cout << "  " << std::endl;
    }



/*
    //郑定义
    std::vector<SatFrameRtn*> powerBuffer1;

    //时隙内译码

    double noisyTotal=0;
    double packetPower=0;
    for(unsigned int i=0;i<mPacketBufferSlot.size(); i++)
    {
        noisyTotal= mPacketBufferSlot[i]->getChannelRatio()*transmitPower;
        noisyTotal+=noisyTotal;
        powerBuffer1.push_back(mPacketBufferSlot[i]->getChannelRatio()*transmitPower);
    }
    for(unsigned int j=0;i<powerButter1.size();j++)
    {
        sort(powerBuffer1[0],powerBuffer1[powerBuffer1.size()-1],greater<int>());//对元素的功率进行排序
        if(!powerBuffer1.empty())
        {
             packetPower=powerButter1.pop();//每次取出最大的功率
        }else{
        }
        double SNRPacket = (packetPower)/(noisyPower+noisyTotal-packetPower);
        if(SNRPacket>=thresholdSNR){
            //std::cout << "SNRPacket = " <<1 << std::endl;
           SatFrameRtn* p00= new SatFrameRtn;
           *p00 = *powerButter1.pop();
            retrievedTemp1.push_back(p00);
            }else{
                //std::cout << "SNRPacket = " <<0 << std::endl;
            }
    }//对接收到的功率值进行排序
*/




/*
    //该时隙内的每个分组进行功率计算
    for(unsigned int i=0; i < mPacketBufferSlot.size(); i++)
    {
        //计算SNR
        double packetPower = mPacketBufferSlot[i]->getChannelRatio()*transmitPower;
        double SNRPacket = (packetPower)/(noisyPower+noisyTotal-packetPower);
        //std::cout << "SNRPacket = " <<SNRPacket << std::endl;
        //判决门限
        if(SNRPacket>=thresholdSNR){

            //std::cout << "SNRPacket = " <<1 << std::endl;
           SatFrameRtn* p00= new SatFrameRtn;
            *p00 = *mPacketBufferSlot[i];
            retrievedTemp1.push_back(p00);

        }else{
            //std::cout << "SNRPacket = " <<0 << std::endl;
        }

    }
    noisyTotal=0;*/


    //去重算法，因为可能有分组的功率计算相同或者同一个用户复制分组在一个时隙内
    //std::cout << "retrievedTemp1 = " <<  retrievedTemp1.size() << std::endl;
    std::vector<int> XOR;
    for(unsigned int i=0; i < retrievedTemp1.size(); i++)
    {
        XOR.push_back(retrievedTemp1[i]->getSrcAddress()*2+retrievedTemp1[i]->getRandomSeed()*3);
    }

    sort(XOR.begin(),XOR.end());
    XOR.erase(unique(XOR.begin(),XOR.end()),XOR.end());

    //std::cout << "XOR = " <<  XOR.size() << std::endl;
    for(unsigned int i=0; i < XOR.size(); i++)
    {
        for(unsigned int j=0; j < retrievedTemp1.size(); j++)
        {
            if(XOR[i]==retrievedTemp1[j]->getSrcAddress()*2+retrievedTemp1[j]->getRandomSeed()*3){

                SatFrameRtn* p01= new SatFrameRtn;
                SatFrameRtn* p02= new SatFrameRtn ;

                *p01 = *retrievedTemp1[j];
                *p02 = *retrievedTemp1[j];
                retrievedPackets.push_back(p01);
                retrievedPacketsAll.push_back(p02);
                break;
            }
        }

    }


/*
//去重算法，存在小bug 使用上面的就行，但是别删除，回头研究一下
    if(retrievedTemp1.size()>1){

        for (int j = 0; j < retrievedTemp1.size()-1; j++)
        {
            int currentAddress = 0, currentSeed = 0;
            if (!retrievedTemp1[j])
            {
                currentAddress = retrievedTemp1[j]->getSrcAddress();
                currentSeed = retrievedTemp1[j]->getRandomSeed();
                for (int k = j + 1; k < retrievedTemp1.size(); k++)
                {
                    if (!retrievedTemp1[k])
                    {
                        if (currentAddress == retrievedTemp1[k]->getSrcAddress() && currentSeed == retrievedTemp1[k]->getRandomSeed())
                        {
                            retrievedTemp1[k] = nullptr;
                            std::cout << j << "  " << k << endl;
                        }
                    }
                }
            }
        }

    }

    std::cout << "retrievedTemp1 = " <<  retrievedTemp1.size() << std::endl;
    for (int j = 0; j < retrievedTemp1.size(); j++)
    {
        if (retrievedTemp1[j] != nullptr)
        {
            SatFrameRtn* temp = new SatFrameRtn;
            *temp = *retrievedTemp1[j];
            retrievedTemp2.push_back(temp);
        }
    }

    std::cout << "retrievedTemp2 = " <<  retrievedTemp2.size() << std::endl;
    for(unsigned int i=0; i < retrievedTemp2.size(); i++)
    {
        SatFrameRtn* p01= new SatFrameRtn;
        SatFrameRtn* p02= new SatFrameRtn ;

        *p01 = *retrievedTemp2[i];
        *p02 = *retrievedTemp2[i];
        retrievedPackets.push_back(p01);
        retrievedPacketsAll.push_back(p02);
    }
*/



    //std::cout << "retrievedPackets= " <<  retrievedPackets.size() << std::endl;

    //retrievedPackets为当前时隙译码出来的分组，在mPacketBuffer中删除早已译码出来的分组，为时隙外准备
    for(unsigned int i=0; i < retrievedPackets.size(); i++)
    {
        //std::cout << "retrievedPackets = " <<  retrievedPackets[i]->getSrcAddress()<<"-"<<retrievedPackets[i]->getRandomSeed() << std::endl;
        int currentSrcAddress = retrievedPackets[i]->getSrcAddress();
        int currentRandomSeed = retrievedPackets[i]->getRandomSeed();
        for(unsigned int j = 0; j < mPacketBuffer.size(); j++)
        {

            //std::cout << "mPacketBuffer = " <<  mPacketBuffer[j]->getSrcAddress()<<"-"<<mPacketBuffer[j]->getRandomSeed() << std::endl;
            if(currentSrcAddress == mPacketBuffer[j]->getSrcAddress() && currentRandomSeed == mPacketBuffer[j]->getRandomSeed())
            {

                delete mPacketBuffer[j];
                mPacketBuffer.erase(mPacketBuffer.begin()+j);
                j--;
            }
        }
    }


    //时隙外译码
    while(mPacketBuffer.size() > 0)
    {
        bool foundSinglePacket = false;
        unsigned int i = 0;
        for(; i < mPacketBuffer.size(); i++)
        {
            int currentSlot = mPacketBuffer[i]->getSlotIndexWithinFrame();
            if(i == mPacketBuffer.size()-1)
            {
                foundSinglePacket = true;
                break;
            }
            else if(currentSlot == mPacketBuffer[i+1]->getSlotIndexWithinFrame())
            {
                do i++; while(i+1 < mPacketBuffer.size() && currentSlot == mPacketBuffer[i+1]->getSlotIndexWithinFrame());
            }
            else
            {
                foundSinglePacket = true;
                break;
            }
        }
        if(!foundSinglePacket)
        {
/*            for(unsigned int j = 0; j < mPacketBuffer.size(); j++)
            delete mPacketBuffer[j];*/
            //mPacketBuffer.clear();
            return retrievedPackets;
        }


        double packetPower = mPacketBuffer[i]->getChannelRatio()*transmitPower;
        double SNRPacket = (packetPower)/noisyPower;
        if(SNRPacket>=thresholdSNROuter){

            int currentSrcAddress = mPacketBuffer[i]->getSrcAddress();
            int currentRandomSeed = mPacketBuffer[i]->getRandomSeed();

            SatFrameRtn* p1= new SatFrameRtn;
            SatFrameRtn* p2= new SatFrameRtn ;

            *p1 = *mPacketBuffer[i];
            *p2 = *mPacketBuffer[i];

            retrievedPackets.push_back(p1);

            retrievedPacketsAll.push_back(p2);

            mPacketBuffer.erase(mPacketBuffer.begin()+i);

            for(unsigned int j = 0; j < mPacketBuffer.size(); j++)
            {
                if(currentSrcAddress == mPacketBuffer[j]->getSrcAddress() && currentRandomSeed == mPacketBuffer[j]->getRandomSeed())
                {
                    delete mPacketBuffer[j];
                    mPacketBuffer.erase(mPacketBuffer.begin()+j);
                    j--;
                }
            }
            //std::cout <<"yes" <<count++ << endl;
        }else{
            //std::cout <<"no"<< count << endl;
            //outputFile << count++ << "\t";
        }

    }

    outputFile.close();
    return retrievedPackets;
}













//结束
void SatGatewayPhy::finish()
{

    std::ofstream outputFile;
    outputFile.open("SimulationResults", std::ios::out | std::ios::app);

    for(unsigned int i = 0; i < mPacketBuffer.size(); i++)
    {
        delete mPacketBuffer[i];
        totalNumberOfPacketsReceived--;
        outputFile << "totalNumberOfPacketsReceived--"<<  std::endl;
    }

    double slot = simTime().dbl()/slotDuration;
    double G = (totalNumberOfPacketsSend / slot) / numberOfReplicas;
    double S = totalNumberOfPacketsReceived / slot;
    delayAvg = delaySum/totalNumberOfPacketsReceived ;

    outputFile << numberOfReplicas << "\t";

    std::cout << "G = " << G << std::endl;
    outputFile << G << "\t";

    std::cout << "S = " << S << std::endl;
    outputFile << S << "\t";

    std::cout << "loss ratio = " << 1-S/G << std::endl;
    outputFile << 1-S/G << "\t";

    std::cout << "delayAvg = " << delayAvg << std::endl;
    outputFile << delayAvg << "\t";

    std::cout << "totalNumberOfPacketsSend = " << totalNumberOfPacketsSend << std::endl;
    outputFile << totalNumberOfPacketsSend << "\t";

    std::cout << "totalNumberOfPacketsReceived = " << totalNumberOfPacketsReceived << std::endl;
    outputFile << totalNumberOfPacketsReceived << std::endl;

    outputFile.close();
}
