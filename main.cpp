#include <iostream>
#include "efp/ElasticFrameProtocol.h"
#include "EFPBonding.h"

#define MTU 1456 //SRT-max
#define NO_INTERFACES 3

#define NO_GROUP_INTERFACES 3
#define NO_GROUPS 1

ElasticFrameProtocol myEFPSender(MTU,ElasticFrameMode::sender);
ElasticFrameProtocol myEFPReceiver;

EFPBonding myEFPBonding;

EFPBonding::EFPBondingInterfaceID interfacesID[NO_INTERFACES];

EFPBonding::EFPBondingInterfaceID groupInterfacesID[NO_GROUP_INTERFACES];
EFPBonding::EFPBondingGroupID groupID[NO_GROUPS];

int groupOfPackets;

struct PrivateData {
  int myPrivateInteger = 10;
  uint8_t myPrivateUint8_t = 44;
  size_t sizeOfData = 0;
};

void sendData(const std::vector<uint8_t> &rSubPacket) {
  myEFPBonding.distributeDataSingle(rSubPacket);
}

void sendDataGroup(const std::vector<uint8_t> &rSubPacket) {
  myEFPBonding.distributeDataGroup(rSubPacket);
}


void gotData(ElasticFrameProtocol::pFramePtr &rPacket) {
  if (rPacket->mBroken) {
    std::cout << "Frame is broken" << std::endl;
    return;
  }

  if (!(rPacket->mPts % 100)) {
    std::cout << "Got packet number " << unsigned(rPacket->mPts - 1000) << std::endl;
    for (int x=0;x<NO_INTERFACES;x++) {
      EFPBonding::EFPStatistics thisInterfaceStatistics = myEFPBonding.getStatistics(interfacesID[x]);
      std::cout << "If: " << unsigned(x) <<
      " fragments: " << unsigned(thisInterfaceStatistics.mNoFragmentsSent) <<
      " part: " << thisInterfaceStatistics.mPercentOfTotalTraffic << "%" <<
      " cover fragments: " << unsigned(thisInterfaceStatistics.mNoGapsCoveredFor) <<
      std::endl;

      if (groupOfPackets == 0) {
        std::cout << "Changing if1 configuration to commit 60% from 40% offset and if2 commit 40% from 0 offset" << std::endl;
        EFPBondingMessages efpBondResult = myEFPBonding.modifyInterfaceCommit(60,0,interfacesID[0]);
        if (efpBondResult != EFPBondingMessages::noError) {
          std::cout << "Failed configuring interface 1." << std::endl;
        }
        efpBondResult = myEFPBonding.modifyInterfaceCommit(40,60,interfacesID[1]);
        if (efpBondResult != EFPBondingMessages::noError) {
          std::cout << "Failed configuring interface 2." << std::endl;
        }
      }
      groupOfPackets++;

    }
    std::cout << "TotalFragments sent: " << myEFPBonding.getGlobalPacketCounter() << std::endl;
  }
}

void gotDataGroup(ElasticFrameProtocol::pFramePtr &rPacket) {
  if (rPacket->mBroken) {
    std::cout << "Frame is broken" << std::endl;
    return;
  }

  if (!(rPacket->mPts % 100)) {
    std::cout << "Got packet number " << unsigned(rPacket->mPts - 1000) << std::endl;
    for (int x=0;x<NO_GROUP_INTERFACES;x++) {
      EFPBonding::EFPStatistics thisInterfaceStatistics = myEFPBonding.getStatistics(groupInterfacesID[x], groupID[0]);
      std::cout << "If: " << unsigned(x) <<
                " fragments: " << unsigned(thisInterfaceStatistics.mNoFragmentsSent) <<
                " part: " << thisInterfaceStatistics.mPercentOfTotalTraffic << "%" <<
                " cover fragments: " << unsigned(thisInterfaceStatistics.mNoGapsCoveredFor) <<
                std::endl;
    }
    std::cout << "TotalFragments sent: " << myEFPBonding.getGlobalPacketCounter() << std::endl;
  }

}

void networkInterface1(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface.
  //Send_if1(rSubPacket);
  //On the recieving side we get the fragment and assemble the superframe
  myEFPReceiver.receiveFragment(rSubPacket,0);
}

void networkInterface2(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface.
  //Send_if2(rSubPacket);
  //On the recieving side we get the fragment and assemble the superframe
  myEFPReceiver.receiveFragment(rSubPacket,0);
}

int dataLoss;
void networkInterface3(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface.
  //Send_if3(rSubPacket);
  //On the recieving side we get the fragment and assemble the superframe
  if (dataLoss++ & 1) {
    myEFPReceiver.receiveFragment(rSubPacket, 0);
  }
}

void networkInterface4(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface.
  //Send_if3(rSubPacket);
  //On the recieving side we get the fragment and assemble the superframe
    myEFPReceiver.receiveFragment(rSubPacket, 0);

}

int main() {

  dataLoss = 0;
  groupOfPackets = 0;

  std::cout << "EFP bonding v" << unsigned((myEFPBonding.getVersion() >> 8) & 0x0f) << "." << unsigned(myEFPBonding.getVersion() & 0x0f) << std::endl;

  //Set-up ElasticFrameProtocol
  myEFPSender.sendCallback = std::bind(&sendData, std::placeholders::_1);
  myEFPReceiver.receiveCallback = std::bind(&gotData, std::placeholders::_1);
  myEFPReceiver.startReceiver(5, 2);

  EFPBonding::EFPInterface lInterface;
  lInterface.mInterfaceLocation = std::bind(&networkInterface1,std::placeholders::_1);
  lInterface.mCommit = 500;
  lInterface.mMasterInterface = MASTER_INTERFACE;
  EFPBonding::EFPBondingInterfaceID ifID = myEFPBonding.addInterface(lInterface);
  if (!ifID) {
    std::cout << "Test1 complete. Failed adding interface" << std::endl;
  } else {
    std::cout << "Test1 failed" << std::endl;
    return EXIT_FAILURE;
  }


  lInterface.mInterfaceLocation = std::bind(&networkInterface1,std::placeholders::_1);
  lInterface.mCommit = 60;
  lInterface.mMasterInterface = MASTER_INTERFACE;
  interfacesID[0] = myEFPBonding.addInterface(lInterface);
  lInterface.mInterfaceLocation = std::bind(&networkInterface2,std::placeholders::_1);
  lInterface.mCommit = 40;
  lInterface.mMasterInterface = NORMAL_INTERFACE;
  interfacesID[1] = myEFPBonding.addInterface(lInterface);
  lInterface.mInterfaceLocation = std::bind(&networkInterface3,std::placeholders::_1);
  lInterface.mCommit = 100;
  lInterface.mMasterInterface = NORMAL_INTERFACE;
  //interfacesID[2] = myEFPBonding.addInterface(lInterface);

 // if (myEFPBonding.mCurrentCoverage != 200) {
 //   std::cout << "Test3 failed" << std::endl;
 //   return EXIT_FAILURE;
 // }

  //std::cout << "Added 3 interfaces (Test1). My coverage is: " << myEFPBonding.mCurrentCoverage << "%" << std::endl;

  //EFPBondingMessages efpBondResult = myEFPBonding.removeInterface(interfacesID[1]);
  //if (myEFPBonding.mCurrentCoverage != 150 || efpBondResult != EFPBondingMessages::noError) {
  //  std::cout << "Test4 failed" << std::endl;
  //  return EXIT_FAILURE;
  //}

  //std::cout << "Removed interface 2 (Test2). My coverage is: " << myEFPBonding.mCurrentCoverage << "%" << std::endl;

  //lInterface.mInterfaceLocation = std::bind(&networkInterface2,std::placeholders::_1);
  //lInterface.mCommit = 40;
  //lInterface.mOffset = 60;
  //lInterface.mMasterInterface = NORMAL_INTERFACE;
  //interfacesID[1] = myEFPBonding.addInterface(lInterface);

  //if (myEFPBonding.mCurrentCoverage != 200) {
  //  std::cout << "Test5 failed" << std::endl;
  //  return EXIT_FAILURE;
  //}

  //If you remove the comments below and change interface interfacesID[1] to offset 0
  //you will get 100% payload but 50% coverage. cover fragments statistics for the master inteface will start counting
  //std::cout << "Added interface 2 again (Test3). My coverage is: " << myEFPBonding.mCurrentCoverage << "%" << std::endl;
  //myEFPBonding.removeInterface(interfacesID[2]);
  //std::cout << "Final coverage: " << myEFPBonding.mCurrentCoverage << "%" << std::endl;

  std::vector<uint8_t> mydata;
  for (int packetNumber=0;packetNumber < 440; packetNumber++) {
    mydata.clear();
    size_t randSize = rand() % 1000000 + 1;
    mydata.resize(randSize);
    std::generate(mydata.begin(), mydata.end(), [n = 0]() mutable { return n++; });
    PrivateData myPrivateData;
    myPrivateData.sizeOfData = mydata.size() + sizeof(PrivateData) + 4; //4 is the embedded frame header size
    myEFPSender.addEmbeddedData(&mydata, &myPrivateData, sizeof(PrivateData), ElasticEmbeddedFrameContent::embeddedprivatedata, true);
    if (myPrivateData.sizeOfData != mydata.size()) {
      std::cout << "Packer error" << std::endl;
    }
    ElasticFrameMessages result = myEFPSender.packAndSend(mydata, ElasticFrameContent::h264, packetNumber+1001, packetNumber+1, EFP_CODE('A', 'N', 'X', 'B'), 4, INLINE_PAYLOAD);
    if (result != ElasticFrameMessages::noError) {
      std::cout << "packAndSend error"
                << std::endl;
    }
  }

  return 0;

  // Group part

  myEFPBonding.clearGlobalPacketCounter();

  //Change the callbacks to the group versions
  myEFPSender.sendCallback = std::bind(&sendDataGroup, std::placeholders::_1);
  myEFPReceiver.receiveCallback = std::bind(&gotDataGroup, std::placeholders::_1);

  //Configure the group interface
  std::vector<EFPBonding::EFPInterface> lInterfaces;

  ifID = myEFPBonding.generateInterfaceID();
  lInterface.mInterfaceID = ifID;
  groupInterfacesID[0] = ifID;
  lInterface.mInterfaceLocation = std::bind(&networkInterface1, std::placeholders::_1);
  lInterface.mMasterInterface = MASTER_INTERFACE;
  lInterfaces.push_back(lInterface);

  ifID = myEFPBonding.generateInterfaceID();
  lInterface.mInterfaceID = ifID;
  groupInterfacesID[1] = ifID;
  lInterface.mInterfaceLocation = std::bind(&networkInterface2, std::placeholders::_1);
  lInterface.mMasterInterface = NORMAL_INTERFACE;
  lInterfaces.push_back(lInterface);

  ifID = myEFPBonding.generateInterfaceID();
  lInterface.mInterfaceID = ifID;
  groupInterfacesID[2] = ifID;
  lInterface.mInterfaceLocation = std::bind(&networkInterface4, std::placeholders::_1);
  lInterface.mMasterInterface = NORMAL_INTERFACE;
  lInterfaces.push_back(lInterface);


  EFPBonding::EFPBondingGroupID bondingGroupID = myEFPBonding.addInterfaceGroup(lInterfaces);
  if (!bondingGroupID) {
    std::cout << "Test6 failed" << std::endl;
    return EXIT_FAILURE;
  }

  groupID[0] = bondingGroupID;


  for (int packetNumber=0;packetNumber < 440; packetNumber++) {
    mydata.clear();
    size_t randSize = rand() % 1000000 + 1;
    mydata.resize(randSize);
    std::generate(mydata.begin(), mydata.end(), [n = 0]() mutable { return n++; });
    PrivateData myPrivateData;
    myPrivateData.sizeOfData = mydata.size() + sizeof(PrivateData) + 4; //4 is the embedded frame header size
    myEFPSender.addEmbeddedData(&mydata, &myPrivateData, sizeof(PrivateData), ElasticEmbeddedFrameContent::embeddedprivatedata, true);
    if (myPrivateData.sizeOfData != mydata.size()) {
      std::cout << "Packer error" << std::endl;
    }
    ElasticFrameMessages result = myEFPSender.packAndSend(mydata, ElasticFrameContent::h264, packetNumber+1001, packetNumber+1, EFP_CODE('A', 'N', 'X', 'B'), 4, INLINE_PAYLOAD);
    if (result != ElasticFrameMessages::noError) {
      std::cout << "packAndSend error"
                << std::endl;
    }
  }

  //efpBondResult = myEFPBonding.removeGroup(groupID[0]);
  //if (efpBondResult != EFPBondingMessages::noError) {
  //  std::cout << "Failed removing group" << std::endl;
  //  return EXIT_FAILURE;
  //}

  return EXIT_SUCCESS;
}