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

EFPBonding::EFPBondingInterfaceID EFPBonding::addInterface(EFPInterface &rInterface) {
  if (rInterface.mCommit > 100 || rInterface.mCommit < 1) {
    return 0;
  }

  if (rInterface.mMasterInterface) {
    mMasterInterfaceLocation = rInterface.mInterfaceLocation;
  }
  std::unique_ptr<EFPInterface> lThisInterface = std::make_unique<EFPInterface>();
  lThisInterface->mInterfaceLocation = rInterface.mInterfaceLocation;
  lThisInterface->mCommit = rInterface.mCommit;
  lThisInterface->mInterfaceID = mUniqueInterfaceID;
  lThisInterface->mMasterInterface = rInterface.mMasterInterface;
  lThisInterface->mPacketCounter = 0;
  lThisInterface->mForwardMissingFragment = 0;
  mInterfaceList.push_back(std::move(lThisInterface));
  mCurrentCoverage = getCoverage();
  return mUniqueInterfaceID++;
}

EFPBondingMessages EFPBonding::removeInterface(EFPBonding::EFPBondingInterfaceID interfaceID) {
  int lInterfaceIndex = 0;
  for (auto const &interface: mInterfaceList) {
    if (interface->mInterfaceID == interfaceID) {
      if (interface->mMasterInterface) {
        mMasterInterfaceLocation = nullptr;
      }
      mInterfaceList.erase(mInterfaceList.begin() + lInterfaceIndex);
      mCurrentCoverage = getCoverage();
      return EFPBondingMessages::noError;
    }
    lInterfaceIndex++;
  }
  return EFPBondingMessages::interfaceIDNotFound;
}

double EFPBonding::getCoverage() {
  double lTotalPercent = 0;
  for (auto const &interface: mInterfaceList) {
    lTotalPercent += interface->mCommit;
  }
  return lTotalPercent;
}

EFPBondingMessages EFPBonding::distributeDataSingle(const std::vector<uint8_t> &rSubPacket) {
  if (mCurrentCoverage < 100.0)
    return EFPBondingMessages::coverageNot100Percent;
  if (mMasterInterfaceLocation == nullptr)
    return EFPBondingMessages::masterInterfaceMissing;

  uint64_t  currentPercentage = mMonotonicPacketCounter % 100;
  if (!currentPercentage) {
    for (auto const &rInterface: mInterfaceList) {
      rInterface->mFireCounter = rInterface->mCommit;
    }
  }
  uint64_t  currentWorker = mMonotonicPacketCounter % mInterfaceList.size();
  mMonotonicPacketCounter++;

  if (mInterfaceList[currentWorker]->mFireCounter) {
    mInterfaceList[currentWorker]->mFireCounter--;
    mInterfaceList[currentWorker]->mInterfaceLocation(rSubPacket);
    mInterfaceList[currentWorker]->mPacketCounter++;
  } else {
    //My worker did not have any capacity left. What about the other interfaces?
    for (auto const &rInterface: mInterfaceList) {
      if (rInterface->mFireCounter) {
        rInterface->mFireCounter--;
        rInterface->mInterfaceLocation(rSubPacket);
        rInterface->mPacketCounter++;
      }
    }


    LOGGER(true, LOGG_WARN, "EFPBonding is not configured for optimal performance.")
    mInterfaceList[currentWorker]->mForwardMissingFragment++;
    mMasterInterfaceLocation(rSubPacket);
  }

  mGlobalPacketCounter++;
  return EFPBondingMessages::noError;
}

EFPBonding::EFPStatistics EFPBonding::getStatistics(EFPBonding::EFPBondingInterfaceID interfaceID,
                                                    EFPBonding::EFPBondingGroupID groupID) {
  EFPBonding::EFPStatistics lMyStatistics;
  if (groupID) {
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
  } else {
    for (auto const &rInterface: mInterfaceList) {
      if (rInterface->mInterfaceID == interfaceID) {
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

EFPBondingMessages EFPBonding::modifyInterfaceCommit(double commit, double offset, EFPBonding::EFPBondingInterfaceID interfaceID, EFPBonding::EFPBondingGroupID groupID) {
  if (commit > 100.0 || offset > 99.0 || (commit + offset) > 100.0 || commit < 0.0) {
    return EFPBondingMessages::errorWhenChangingCommit;
  }

  for (auto const &rInterface: mInterfaceList) {
    if (rInterface->mInterfaceID == interfaceID) {
      rInterface->mCommit = commit;
      rInterface->mFireCounter = offset / 100.0;
      return EFPBondingMessages::noError;
    }
  }
  return EFPBondingMessages::interfaceIDNotFound;
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
      mInterfaceList.erase(mInterfaceList.begin() + lGroupIndex);
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
  mGlobalPacketCounter++;
  for (auto const &rInterfaces: mGroupList) {
    std::function<void(const std::vector<uint8_t> &)> lMasterInterfaceLocation = nullptr;
    uint64_t *pForwardMissingFragment = nullptr;
    bool lDidForwardFragment = false;
    for (auto const &rInterface: rInterfaces) {
      if (rInterface->mMasterInterface) {
        lMasterInterfaceLocation = rInterface->mInterfaceLocation;
        pForwardMissingFragment = &rInterface->mForwardMissingFragment;
      }
      rInterface->mFireCounter += rInterface->mCommit / 100.0;
      if (rInterface->mFireCounter >= 1.0) {
        lDidForwardFragment = true;
        rInterface->mInterfaceLocation(rSubPacket);
        rInterface->mFireCounter -= 1.0;
        rInterface->mPacketCounter++;
      }
    }
    if (!lDidForwardFragment) {
      pForwardMissingFragment[0]++;
      lMasterInterfaceLocation(rSubPacket);
    }

  }
  return EFPBondingMessages::noError;
}

