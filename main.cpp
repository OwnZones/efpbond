#include <iostream>
#include "efp/ElasticFrameProtocol.h"
#include "EFPBonding.h"

#define MTU 1456 //SRT-max

#define NO_GROUP_INTERFACES 3
#define NO_GROUPS 2

//EFP
ElasticFrameProtocolSender myEFPSender(MTU);
ElasticFrameProtocolReceiver myEFPReceiver(50, 20);

//EFP Bonding plug-in
EFPBonding myEFPBonding;

EFPBonding::EFPBondingInterfaceID groupInterfacesID[NO_GROUP_INTERFACES];
EFPBonding::EFPBondingGroupID groupID[NO_GROUPS];

int groupOfPackets;
int superframeCounter;
int testNumber;

void efpBondTestsHandler1();
void efpBondTestsHandler2();
void efpBondTestsHandler3();

struct PrivateData {
  int myPrivateInteger = 10;
  uint8_t myPrivateUint8_t = 44;
  size_t sizeOfData = 0;
};

//This is the same thread as EFP packAndSend we can only call EFPBond methods from this thread since EFPBond is not thread safe!!
void sendData(const std::vector<uint8_t> &rSubPacket, uint8_t fragmentID) {
  //If you want to keep track of the statistics. Then call this every time a fragment is sent
  myEFPBonding.increaseGlobalPacketCounter();
  //If splitData recieves 0 as fragmentID then split mode will not be active
  //We only want to split data in test 3
  if (testNumber == 3) {
    EFPBondingMessages result = myEFPBonding.splitData(rSubPacket, fragmentID);
    if (result == EFPBondingMessages::fragmentNotSent) {
      std::cout << "This fragment was not sent on any interface. Discard or hand to a interface?" << std::endl;
    }
  }
  //distributeData does not use fragment id
  myEFPBonding.distributeData(rSubPacket, 0);
  if ((rSubPacket[0] & 0x0f) == 2) {
    superframeCounter++;
    if (testNumber == 1) efpBondTestsHandler1();
    if (testNumber == 2) efpBondTestsHandler2();
    if (testNumber == 3) efpBondTestsHandler3();
  }
}

void efpBondTestsHandler1() {
  if (!(superframeCounter % 100)) {
    std::cout << "Did send frame " << unsigned(superframeCounter) << std::endl;
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

      std::vector<EFPBonding::EFPInterfaceCommit> myInterfaceCommits;
      myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(20, groupInterfacesID[1]));
      myEFPBonding.modifyInterfaceCommits(myInterfaceCommits, groupID[0]);
    } else if (groupOfPackets == 1) {
      std::vector<EFPBonding::EFPInterfaceCommit> myInterfaceCommits;
      myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(50, groupInterfacesID[0]));
      myEFPBonding.modifyInterfaceCommits(myInterfaceCommits, groupID[0]);
    } else if (groupOfPackets == 2) {
      std::vector<EFPBonding::EFPInterfaceCommit> myInterfaceCommits;
      myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(25, groupInterfacesID[0]));
      myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(25, groupInterfacesID[1]));
      myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(50, groupInterfacesID[2]));
      myEFPBonding.modifyInterfaceCommits(myInterfaceCommits, groupID[0]);
    }
    groupOfPackets++;
  }
}

void efpBondTestsHandler2() {
  if (!(superframeCounter % 100)) {
    std::cout << "Did send frame " << unsigned(superframeCounter) << std::endl;
    for (int x=0;x<2;x++) {
      EFPBonding::EFPStatistics thisInterfaceStatistics = myEFPBonding.getStatistics(groupInterfacesID[x], groupID[0], true);
      std::cout << "If: " << unsigned(x) <<
                " fragments: " << unsigned(thisInterfaceStatistics.mNoFragmentsSent) <<
                " part: " << thisInterfaceStatistics.mPercentOfTotalTraffic << "%" <<
                " cover fragments: " << unsigned(thisInterfaceStatistics.mNoGapsCoveredFor) <<
                std::endl;
    }

    EFPBonding::EFPStatistics thisInterfaceStatistics = myEFPBonding.getStatistics(groupInterfacesID[2], groupID[1], true);
    std::cout << "If: " << unsigned(3) <<
              " fragments: " << unsigned(thisInterfaceStatistics.mNoFragmentsSent) <<
              " part: " << thisInterfaceStatistics.mPercentOfTotalTraffic << "%" <<
              " cover fragments: " << unsigned(thisInterfaceStatistics.mNoGapsCoveredFor) <<
              std::endl;


    std::cout << "TotalFragments sent: " << myEFPBonding.getGlobalPacketCounter() << std::endl;
    myEFPBonding.clearGlobalPacketCounter();
  }
}

void efpBondTestsHandler3() {
  if (!(superframeCounter % 100)) {
    EFPBonding::EFPStatistics splitInterface1Stats = myEFPBonding.getStatistics(groupInterfacesID[0], 0, true);
    EFPBonding::EFPStatistics splitInterface2Stats = myEFPBonding.getStatistics(groupInterfacesID[1], 0, true);
    std::cout << "Did send frame " << unsigned(superframeCounter) << std::endl;

    std::cout << "Split If1: " <<
              " fragments: " << splitInterface1Stats.mNoFragmentsSent <<
              std::endl;

    std::cout << "Split If2: " <<
              " fragments: " << splitInterface2Stats.mNoFragmentsSent <<
              std::endl;

    std::cout << "TotalFragments If1+If2: " << splitInterface1Stats.mNoFragmentsSent+splitInterface2Stats.mNoFragmentsSent << std::endl;
    std::cout << "TotalFragments from EFP: " << myEFPBonding.getGlobalPacketCounter() << std::endl;
    myEFPBonding.clearGlobalPacketCounter();
  }
}

void gotData(ElasticFrameProtocolReceiver::pFramePtr &rPacket) {
  if (rPacket->mBroken) {
    std::cout << "Frame is broken" << std::endl;
    return;
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
  superframeCounter = 0;
  testNumber = 1;

  std::cout << "EFP bonding v" << unsigned((myEFPBonding.getVersion() >> 8) & 0x0f) << "." << unsigned(myEFPBonding.getVersion() & 0x0f) << std::endl;

  std::cout << std::endl <<"Test1 Bonding of 3 interfaces" << std::endl << std::endl;

  //Set-up ElasticFrameProtocol
  myEFPSender.sendCallback = std::bind(&sendData, std::placeholders::_1, std::placeholders::_2);
  myEFPReceiver.receiveCallback = std::bind(&gotData, std::placeholders::_1);

    //Group interface
    std::vector<EFPBonding::EFPInterface> lInterfaces;
    //Get ID from EFPBonding generate ID
    groupInterfacesID[0] = myEFPBonding.generateInterfaceID();
    //Generate and store that interface in the list of interfaces
    lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface1, std::placeholders::_1),
                                                   groupInterfacesID[0],
                                                   EFP_MASTER_INTERFACE));
    groupInterfacesID[1] = myEFPBonding.generateInterfaceID();
    lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface2, std::placeholders::_1),
                                                   groupInterfacesID[1],
                                                   EFP_NORMAL_INTERFACE));
    groupInterfacesID[2] = myEFPBonding.generateInterfaceID();
    lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface4, std::placeholders::_1),
                                                   groupInterfacesID[2],
                                                   EFP_NORMAL_INTERFACE));

    //When done push the interfaces to EFPBonding creating a EFPBonding-group. The return is a groupID
    groupID[0] = myEFPBonding.addInterfaceGroup(lInterfaces);

    if (!groupID[0]) {
      std::cout << "Test1 failed" << std::endl;
      return EXIT_FAILURE;
    }

    //Send data
    std::vector<uint8_t> mydata;
    for (int packetNumber = 0; packetNumber < 440; packetNumber++) {
      mydata.clear();
      size_t randSize = rand() % 1000000 + 1;
      mydata.resize(randSize);
      std::generate(mydata.begin(), mydata.end(), [n = 0]() mutable { return n++; });
      PrivateData myPrivateData;
      myPrivateData.sizeOfData = mydata.size() + sizeof(PrivateData) + 4; //4 is the embedded frame header size
      myEFPSender.addEmbeddedData(&mydata,
                                  &myPrivateData,
                                  sizeof(PrivateData),
                                  ElasticEmbeddedFrameContent::embeddedprivatedata,
                                  true);
      if (myPrivateData.sizeOfData != mydata.size()) {
        std::cout << "Packer error" << std::endl;
      }
      ElasticFrameMessages result = myEFPSender.packAndSend(mydata,
                                                            ElasticFrameContent::h264,
                                                            packetNumber + 1001,
                                                            packetNumber + 1,
                                                            EFP_CODE('A', 'N', 'X', 'B'),
                                                            4,
                                                            INLINE_PAYLOAD);
      //Since EFPBond is not thread safe we need to call all methods from the same thread as from EFP packAndSend.
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

  std::cout << std::endl <<"Test2 Bonding of 2 interfaces then 1 interface with 50% loss is part of a 1+1 set-up" << std::endl << std::endl;

    //When here we have
    //1. created a group of 3 interfaces sending 100 superframes load balance 33.3% for each interface
    //2. Then we changed interface 2 to commit to 20% and sent another 100 superframes
    //3. After that We modifyed all interfaces to if1 25% if2 25% if3 50% and sent another 100 superframes
    //4. We then removed the group.

    //Below we create 3 interfaces two bonded 50% load each and then a single interface..
    //The single interface is covering for 100% load self but this interface is also dropping 50% of the transported payload.
    //so we generated a 1+1 and test EFP + EFPBond to see if the data is truly protected.

    //Crear stats (we're on the same thread as packAndSend so we're fine)
    myEFPBonding.clearGlobalPacketCounter();
    superframeCounter = 0;
    testNumber = 2;

    //Remove all interfaces
    lInterfaces.clear();

    //Build new interfaces
    groupInterfacesID[0] = myEFPBonding.generateInterfaceID();
    lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface1, std::placeholders::_1),
                                                   groupInterfacesID[0],
                                                   EFP_MASTER_INTERFACE));
    groupInterfacesID[1] = myEFPBonding.generateInterfaceID();
    lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface2, std::placeholders::_1),
                                                   groupInterfacesID[1],
                                                   EFP_NORMAL_INTERFACE));
    //When done push the interfaces to EFPBonding creating a EFPBonding-group (2 interfaces 50% payload each.)
    groupID[0] = myEFPBonding.addInterfaceGroup(lInterfaces);
    if (!groupID[0]) {
      std::cout << "Test2 failed" << std::endl;
      return EXIT_FAILURE;
    }

    //Remove all interfaces
    lInterfaces.clear();

    //Build new interface
    groupInterfacesID[2] = myEFPBonding.generateInterfaceID();
    lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface1, std::placeholders::_1),
                                                   groupInterfacesID[2],
                                                   EFP_MASTER_INTERFACE));

    groupID[1] = myEFPBonding.addInterfaceGroup(lInterfaces);
    if (!groupID[1]) {
      std::cout << "Test2 failed" << std::endl;
      return EXIT_FAILURE;
    }

    //Ok we created our bonding now let's send some data
    for (int packetNumber = 0; packetNumber < 340; packetNumber++) {
      mydata.clear();
      size_t randSize = rand() % 1000000 + 1;
      mydata.resize(randSize);
      std::generate(mydata.begin(), mydata.end(), [n = 0]() mutable { return n++; });
      PrivateData myPrivateData;
      myPrivateData.sizeOfData = mydata.size() + sizeof(PrivateData) + 4; //4 is the embedded frame header size
      myEFPSender.addEmbeddedData(&mydata,
                                  &myPrivateData,
                                  sizeof(PrivateData),
                                  ElasticEmbeddedFrameContent::embeddedprivatedata,
                                  true);
      if (myPrivateData.sizeOfData != mydata.size()) {
        std::cout << "Packer error" << std::endl;
      }
      ElasticFrameMessages result = myEFPSender.packAndSend(mydata,
                                                            ElasticFrameContent::h264,
                                                            packetNumber + 1001,
                                                            packetNumber + 1,
                                                            EFP_CODE('A', 'N', 'X', 'B'),
                                                            4,
                                                            INLINE_PAYLOAD);
      //Since EFPBond is not thread safe we need to call all methods from the same thread as from EFP packAndSend.
      if (result != ElasticFrameMessages::noError) {
        std::cout << "packAndSend error" << std::endl;
        return EXIT_FAILURE;
      }
    }

    efpBondResult = myEFPBonding.removeGroup(groupID[0]);
    if (efpBondResult != EFPBondingMessages::noError) {
      std::cout << "Failed removing group" << std::endl;
      return EXIT_FAILURE;
    }
    efpBondResult = myEFPBonding.removeGroup(groupID[1]);
    if (efpBondResult != EFPBondingMessages::noError) {
      std::cout << "Failed removing group" << std::endl;
      return EXIT_FAILURE;
    }

  std::cout << std::endl <<"Test3 Splitting 3 EFP streams over 2 interfaces" << std::endl << std::endl;

  //Crear stats (we're on the same thread as packAndSend so we're fine)
  myEFPBonding.clearGlobalPacketCounter();
  superframeCounter = 0;
  testNumber = 3;

  //Split interface Tests
  //
  // Below we split 3 EFP streams.
  // EFP Stream 10 and 20 is sent to interface networkInterface1 (we call this proxy interface splitInterface1)
  // EFP Stream 30 is sent to networkInterface2

  //Setup interfaces
  std::vector<uint8_t> lEFPIDs = {10,20};
  groupInterfacesID[0] = myEFPBonding.generateInterfaceID();
  EFPBonding::EFPInterface splitInterface1 = EFPBonding::EFPInterface(std::bind(&networkInterface1, std::placeholders::_1),groupInterfacesID[0]);
  myEFPBonding.addInterfaceToStreamID(splitInterface1,lEFPIDs);
  lEFPIDs = {30};
  groupInterfacesID[1] = myEFPBonding.generateInterfaceID();
  EFPBonding::EFPInterface splitInterface2 = EFPBonding::EFPInterface(std::bind(&networkInterface2, std::placeholders::_1),groupInterfacesID[1]);
  myEFPBonding.addInterfaceToStreamID(splitInterface2,lEFPIDs);

  //Ok we created our split interfaces
  uint8_t senderEFPID[3] = {10,20,30};
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
    ElasticFrameMessages result = myEFPSender.packAndSend(mydata, ElasticFrameContent::h264, packetNumber+1001, packetNumber+1, EFP_CODE('A', 'N', 'X', 'B'), senderEFPID[packetNumber % 3], INLINE_PAYLOAD);
    //Since EFPBond is not thread safe we need to call all methods from the same thread as from EFP packAndSend.
    if (result != ElasticFrameMessages::noError) {
      std::cout << "packAndSend error" << std::endl;
      return EXIT_FAILURE;
    }
  }

  //Remove interfaces
  lEFPIDs = {10,20};
  myEFPBonding.removeInterfaceFromStreamID(groupInterfacesID[0],lEFPIDs);
  lEFPIDs = {30};
  myEFPBonding.removeInterfaceFromStreamID(groupInterfacesID[1],lEFPIDs);

  return EXIT_SUCCESS;
}