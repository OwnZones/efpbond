#include <iostream>
#include "efp/ElasticFrameProtocol.h"
#include "EFPBonding.h"

#define MTU 1456 //SRT-max
#define NO_INTERFACES 3

ElasticFrameProtocol myEFPSender(MTU,ElasticFrameMode::sender);
ElasticFrameProtocol myEFPReceiver;

EFPBonding myEFPBonding;

EFPBonding::EFPBondingID interfacesID[NO_INTERFACES];

struct PrivateData {
  int myPrivateInteger = 10;
  uint8_t myPrivateUint8_t = 44;
  size_t sizeOfData = 0;
};

void sendData(const std::vector<uint8_t> &rSubPacket) {
  myEFPBonding.distributeData(rSubPacket);
}

void gotData(ElasticFrameProtocol::pFramePtr &rPacket) {
  if (rPacket->mBroken) {
    std::cout << "Crap" << std::endl;
    return;
  }

  if (!(rPacket->mPts % 100)) {
    std::cout << "Got packet number " << unsigned(rPacket->mPts - 1000) << std::endl;
    for (int x=0;x<NO_INTERFACES;x++) {
      EFPBonding::EFPStatistics thisInterfaceStatistics = myEFPBonding.getStatistics(interfacesID[x]);
      std::cout << "If: " << unsigned(x) <<
      " fragments: " << unsigned(thisInterfaceStatistics.noFragmentsSent) <<
      " part: " << thisInterfaceStatistics.percentOfTotalTraffic << "%" <<
      " cover fragments: " << unsigned(thisInterfaceStatistics.noGapsCoveredFor) << std::endl;
    }
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

int main() {

  dataLoss = 0;

  std::cout << "EFP bonding v" << unsigned((myEFPBonding.getVersion() >> 8) & 0x0f) << "." << unsigned(myEFPBonding.getVersion() & 0x0f) << std::endl;

  //Set-up ElasticFrameProtocol
  myEFPSender.sendCallback = std::bind(&sendData, std::placeholders::_1);
  myEFPReceiver.receiveCallback = std::bind(&gotData, std::placeholders::_1);
  myEFPReceiver.startReceiver(5, 2);


  EFPBonding::EFPBondingID ifID = myEFPBonding.addInterface(std::bind(&networkInterface1,std::placeholders::_1), 500, 0, MASTER_INTERFACE);
  if (!ifID) {
    std::cout << "Test1 complete. Failed adding interface" << std::endl;
  } else {
    std::cout << "Test1 failed" << std::endl;
    return EXIT_FAILURE;
  }
  ifID = myEFPBonding.addInterface(std::bind(&networkInterface1,std::placeholders::_1), 50, 500, MASTER_INTERFACE);
  if (!ifID) {
    std::cout << "Test2 complete. Failed adding interface" << std::endl;
  } else {
    std::cout << "Test1 failed" << std::endl;
    return EXIT_FAILURE;
  }

  interfacesID[0] = myEFPBonding.addInterface(std::bind(&networkInterface1,std::placeholders::_1), 50, 0, MASTER_INTERFACE);

  interfacesID[1] = myEFPBonding.addInterface(std::bind(&networkInterface2,std::placeholders::_1), 50, 50, NORMAL_INTERFACE);

  interfacesID[2] = myEFPBonding.addInterface(std::bind(&networkInterface3,std::placeholders::_1), 100, 0, NORMAL_INTERFACE);

  if (myEFPBonding.currentCoverage != 200) {
    std::cout << "Test3 failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Added 3 interfaces (Test1). My coverage is: " << myEFPBonding.currentCoverage << "%" << std::endl;

  myEFPBonding.removeInterface(interfacesID[1]);

  if (myEFPBonding.currentCoverage != 150) {
    std::cout << "Test4 failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Removed interface 2 (Test2). My coverage is: " << myEFPBonding.currentCoverage << "%" << std::endl;

  interfacesID[1] = myEFPBonding.addInterface(std::bind(&networkInterface2,std::placeholders::_1), 50, 0, NORMAL_INTERFACE);

  if (myEFPBonding.currentCoverage != 200) {
    std::cout << "Test5 failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Added interface 2 again (Test3). My coverage is: " << myEFPBonding.currentCoverage << "%" << std::endl;

  myEFPBonding.removeInterface(interfacesID[2]);
  std::cout << "Final coverage: " << myEFPBonding.currentCoverage << "%" << std::endl;

  std::vector<uint8_t> mydata;
  for (int packetNumber=0;packetNumber < 1000; packetNumber++) {
    mydata.clear();

    // std::cout << "bip " << unsigned(packetNumber) << std::endl;

    size_t randSize = rand() % 1000000 + 1;
    //size_t randSize = (MTU*2-(myEFPPacker.geType1Size()*2)-(1+sizeof(PrivateData) + 4));
    mydata.resize(randSize);
    std::generate(mydata.begin(), mydata.end(), [n = 0]() mutable { return n++; });

    PrivateData myPrivateData;
    myPrivateData.sizeOfData = mydata.size() + sizeof(PrivateData) + 4; //4 is the embedded frame header size
    myEFPSender.addEmbeddedData(&mydata, &myPrivateData, sizeof(PrivateData), ElasticEmbeddedFrameContent::embeddedprivatedata, true);
    if (myPrivateData.sizeOfData != mydata.size()) {
      std::cout << "Packer error"
                << std::endl;
    }

    ElasticFrameMessages result = myEFPSender.packAndSend(mydata, ElasticFrameContent::h264, packetNumber+1001, packetNumber+1, EFP_CODE('A', 'N', 'X', 'B'), 4, INLINE_PAYLOAD);
    if (result != ElasticFrameMessages::noError) {
      std::cout << "packAndSend error"
                << std::endl;
    }
  }


  return 0;
}