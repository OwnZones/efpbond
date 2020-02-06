#include <iostream>
#include "efp/ElasticFrameProtocol.h"
#include "EFPBonding.h"

#define MTU 1456 //SRT-max

#define NO_GROUP_INTERFACES 3
#define NO_GROUPS 1

//EFP
ElasticFrameProtocol myEFPSender(MTU,ElasticFrameMode::sender);
ElasticFrameProtocol myEFPReceiver;

//EFP Bonding plug-in
EFPBonding myEFPBonding;

EFPBonding::EFPBondingInterfaceID groupInterfacesID[NO_GROUP_INTERFACES];
EFPBonding::EFPBondingGroupID groupID[NO_GROUPS];

int groupOfPackets;

struct PrivateData {
  int myPrivateInteger = 10;
  uint8_t myPrivateUint8_t = 44;
  size_t sizeOfData = 0;
};

void sendData(const std::vector<uint8_t> &rSubPacket) {
  myEFPBonding.distributeDataGroup(rSubPacket);
}


void gotData(ElasticFrameProtocol::pFramePtr &rPacket) {
  if (rPacket->mBroken) {
    std::cout << "Frame is broken" << std::endl;
    return;
  }

  if (!(rPacket->mPts % 100)) {
    std::cout << "Got packet number " << unsigned(rPacket->mPts - 1000) << std::endl;
    for (int x=0;x<NO_GROUP_INTERFACES;x++) {
      EFPBonding::EFPStatistics thisInterfaceStatistics = myEFPBonding.getStatistics(groupInterfacesID[x], groupID[0], true);
      std::cout << "If: " << unsigned(x) <<
                " fragments: " << unsigned(thisInterfaceStatistics.mNoFragmentsSent) <<
                " part: " << thisInterfaceStatistics.mPercentOfTotalTraffic << "%" <<
                " cover fragments: " << unsigned(thisInterfaceStatistics.mNoGapsCoveredFor) <<
                std::endl;
    }
    std::cout << "TotalFragments sent: " << myEFPBonding.getGlobalPacketCounter() << std::endl;

    myEFPBonding.clearGlobalPacketCounter();

    if (groupOfPackets == 0) {
      EFPBonding::EFPInterfaceCommit myInterfaceCommit;
      myInterfaceCommit.mCommit = 20;
      myInterfaceCommit.mGroupID = groupID[0];
      myInterfaceCommit.mInterfaceID = groupInterfacesID[1];
      myEFPBonding.modifyInterfaceCommit(myInterfaceCommit);

    } else if (groupOfPackets == 1) {
      std::vector<EFPBonding::EFPInterfaceCommit> myInterfaceCommits;
      EFPBonding::EFPInterfaceCommit myInterfaceCommit;
      myInterfaceCommit.mCommit = 25;
      myInterfaceCommit.mGroupID = groupID[0];
      myInterfaceCommit.mInterfaceID = groupInterfacesID[0];
      myInterfaceCommits.push_back(myInterfaceCommit);
      myInterfaceCommit.mCommit = 25;
      myInterfaceCommit.mGroupID = groupID[0];
      myInterfaceCommit.mInterfaceID = groupInterfacesID[1];
      myInterfaceCommits.push_back(myInterfaceCommit);
      myInterfaceCommit.mCommit = 50;
      myInterfaceCommit.mGroupID = groupID[0];
      myInterfaceCommit.mInterfaceID = groupInterfacesID[2];
      myInterfaceCommits.push_back(myInterfaceCommit);
      myEFPBonding.modifyTotalGroupCommit(myInterfaceCommits);
    }

    groupOfPackets++;

  }

}

void networkInterface1(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface. 0% loss
  //Send_if1(rSubPacket);
  //On the recieving side we get the fragment and assemble the superframe
  myEFPReceiver.receiveFragment(rSubPacket,0);
}

void networkInterface2(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface. 0% loss
  //Send_if2(rSubPacket);
  //On the recieving side we get the fragment and assemble the superframe
  myEFPReceiver.receiveFragment(rSubPacket,0);
}

int dataLoss;
void networkInterface3(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface. 50% loss
  //Send_if3(rSubPacket);
  //On the recieving side we get the fragment and assemble the superframe
  if (dataLoss++ & 1) {
    myEFPReceiver.receiveFragment(rSubPacket, 0);
  }
}

void networkInterface4(const std::vector<uint8_t> &rSubPacket) {
  //Send the data trough this interface. 0% loss
  //Send_if4(rSubPacket);
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

  //Interface
  EFPBonding::EFPInterface lInterface;
  //Group interface
  std::vector<EFPBonding::EFPInterface> lInterfaces;

  //Get a ID from EFPBonding
  EFPBonding::EFPBondingInterfaceID ifID = myEFPBonding.generateInterfaceID();
  //I assign that but also store it so that I can target this interface later
  lInterface.mInterfaceID = ifID;
  groupInterfacesID[0] = ifID;
  //I provide the callback where the 'sendto' is located
  lInterface.mInterfaceLocation = std::bind(&networkInterface1, std::placeholders::_1);

  //At least one interface has to be a master interface in the group
  lInterface.mMasterInterface = MASTER_INTERFACE;

  //Put the interface into a vector of interfaces
  lInterfaces.push_back(lInterface);

  //Do the same for the other interfaces
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


  //When done push the interfaces to EFPBonding creating a EFPBonding-group
  EFPBonding::EFPBondingGroupID bondingGroupID = myEFPBonding.addInterfaceGroup(lInterfaces);
  if (!bondingGroupID) {
    std::cout << "Test1 failed" << std::endl;
    return EXIT_FAILURE;
  }

  //Store the group ID for controlling the group later.
  groupID[0] = bondingGroupID;

  //Send data
  std::vector<uint8_t> mydata;
  for (int packetNumber=0;packetNumber < 340; packetNumber++) {
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
      std::cout << "packAndSend error" << std::endl;
      return EXIT_FAILURE;
    }
  }

  EFPBondingMessages efpBondResult = myEFPBonding.removeGroup(groupID[0]);
  if (efpBondResult != EFPBondingMessages::noError) {
    std::cout << "Failed removing group" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}