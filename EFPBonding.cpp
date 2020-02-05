//
// UnitX Edgeware AB 2020
//

#include "EFPBonding.h"
#include "Bonding.h"

//Constructor
EFPBonding::EFPBonding() {
  LOGGER(true, LOGG_NOTIFY, "EFPBonding constructed")
  mGlobalPacketCounter = 0;
}

//Destructor
EFPBonding::~EFPBonding() {
  LOGGER(true, LOGG_NOTIFY, "EFPBonding destruct")
}

EFPBondingMessages EFPBonding::modifyInterfaceCommit(double commit,
                                                     EFPBonding::EFPBondingInterfaceID interfaceID,
                                                     EFPBonding::EFPBondingGroupID groupID) {

  return EFPBondingMessages::interfaceIDNotFound;
}

EFPBonding::EFPStatistics EFPBonding::getStatistics(EFPBonding::EFPBondingInterfaceID interfaceID,
                                                    EFPBonding::EFPBondingGroupID groupID) {
  EFPBonding::EFPStatistics lMyStatistics;

  if (!interfaceID || !groupID || !mGroupList.size()) {
    return lMyStatistics;
  }
  for (auto const &rInterfaces: mGroupList) {
    for (auto const &rInterface: rInterfaces) {
      if (rInterface->mInterfaceID == interfaceID && rInterface->mGroupID == groupID) {
        lMyStatistics.mNoFragmentsSent = rInterface->mPacketCounter;
        lMyStatistics.mNoGapsCoveredFor = rInterface->mForwardMissingFragment;
        lMyStatistics.mPercentOfTotalTraffic =
            ((double) rInterface->mPacketCounter / (double) mGlobalPacketCounter) * 100.0;
        return lMyStatistics;
      }
    }
  }
  return lMyStatistics;
}

uint64_t EFPBonding::getGlobalPacketCounter() {
  return mGlobalPacketCounter;
}

void EFPBonding::clearGlobalPacketCounter() {
  mGlobalPacketCounter = 0;
}

EFPBonding::EFPBondingInterfaceID EFPBonding::generateInterfaceID() {
  return mUniqueInterfaceID++;
}


// ------------------------ Group methods


EFPBonding::EFPBondingGroupID EFPBonding::addInterfaceGroup(std::vector<EFPInterface> &rInterfaces) {
  if (!rInterfaces.size() || rInterfaces.size() == 1) { //We need at least two interfaces to build a group
    return 0;
  }

  //Auto assign offset and coverage
  double lCommit = 100.0 / (double) rInterfaces.size();
  double lOffset = 0.0;
  bool didProvideMasterInterface = false;
  std::vector<std::unique_ptr<EFPInterface>> lGroupList;
  for (auto const &rInterface: rInterfaces) {
    if (rInterface.mMasterInterface) {
      didProvideMasterInterface = true;
    }
    std::unique_ptr<EFPInterface> lThisInterface = std::make_unique<EFPInterface>();
    lThisInterface->mInterfaceLocation = rInterface.mInterfaceLocation;
    lThisInterface->mCommit = lCommit;
    lThisInterface->mFireCounter = lOffset / 100.0;
    lThisInterface->mInterfaceID = rInterface.mInterfaceID;
    lThisInterface->mGroupID = mUniqueGroupID;
    lThisInterface->mMasterInterface = rInterface.mMasterInterface;
    lThisInterface->mPacketCounter = 0;
    lThisInterface->mForwardMissingFragment = 0;
    lGroupList.push_back(std::move(lThisInterface));
    lOffset += lCommit;
  }

  if (!didProvideMasterInterface) {
    LOGGER(true, LOGG_ERROR, "Did not provide master interface")
    return 0;
  }

  mGroupList.push_back(std::move(lGroupList));
  return mUniqueGroupID++;
}

EFPBondingMessages EFPBonding::removeGroup(EFPBondingGroupID groupID) {
  int lGroupIndex = 0;
  for (auto const &interfaceList: mGroupList) {
    if (interfaceList[0]->mInterfaceID == groupID) {
      mGroupList.erase(mGroupList.begin() + lGroupIndex);
      return EFPBondingMessages::noError;
    }
    lGroupIndex++;
  }
  return EFPBondingMessages::removeGroupNotFound;

}

EFPBondingMessages EFPBonding::distributeDataGroup(const std::vector<uint8_t> &rSubPacket) {
  if (!mGroupList.size()) {
    return EFPBondingMessages::noGroupsFound;
  }

  uint64_t currentPercentage = mMonotonicPacketCounter % 100;
  if (!currentPercentage) {
    for (auto const &rInterfaces: mGroupList) {
      for (auto const &rInterface: rInterfaces) {
        rInterface->mFireCounter += rInterface->mCommit;
      }
    }
  }

  for (auto const &rInterfaces: mGroupList) {

    bool didSendSomething = false;
    for (auto const &rInterface: rInterfaces) {
      if (rInterface->mFireCounter >= 1.0) {
        didSendSomething = true;
        rInterface->mFireCounter--;
        rInterface->mInterfaceLocation(rSubPacket);
        rInterface->mPacketCounter++;
        break;
      }
    }

    if (!didSendSomething) {
      for (auto const &rInterface: rInterfaces) {
        if (rInterface->mMasterInterface) {
          rInterface->mInterfaceLocation(rSubPacket);
          rInterface->mPacketCounter++;
          rInterface->mForwardMissingFragment++;
        }
      }
    }
  }

  mGlobalPacketCounter++;
  mMonotonicPacketCounter++;

  return EFPBondingMessages::noError;
}

