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

//FIXME - Code is not optimized.
EFPBondingMessages EFPBonding::modifyInterfaceCommit(EFPBonding::EFPInterfaceCommit &rInterfaceCommit) {

  if (rInterfaceCommit.mCommit < 1.0 || rInterfaceCommit.mCommit > 100.0 || !rInterfaceCommit.mGroupID
      || !rInterfaceCommit.mInterfaceID) {
    return EFPBondingMessages::parameterError;
  }

  std::vector<std::shared_ptr<EFPInterface>> lThisGroupsInterfaces;
  double lSumOfCommits = 0;
  double lNewCommitDiff = 0;
  bool foundInterface = false;

  //FIXME remove all debug when feeling confident
  double forDebuggingCalculation = 0;

  for (auto const &rInterfaces: mGroupList) {
    for (auto const &rInterface: rInterfaces) {
      if (rInterface->mGroupID == rInterfaceCommit.mGroupID) {
        if (rInterface->mInterfaceID != rInterfaceCommit.mInterfaceID) {
          lSumOfCommits += rInterface->mCommit;
          lThisGroupsInterfaces.push_back(rInterface);
        } else {
          foundInterface = true;
          lNewCommitDiff = rInterfaceCommit.mCommit - rInterface->mCommit;
          rInterface->mCommit = rInterfaceCommit.mCommit;
          forDebuggingCalculation = rInterface->mCommit;
        }
      }
    }
  }

  if (!foundInterface) {
    return EFPBondingMessages::interfaceIDNotFound;
  }

  //recalculate the commitment for the other interfaces based on their current relative commitment

  for (auto const &rInterface: lThisGroupsInterfaces) {
    //If a interface has 0% commit then you can't ask nothing to become something without forcing it.
    if (rInterface->mCommit > 0.0) {
      double lCurrentWeight = lSumOfCommits / rInterface->mCommit;
      double lRelativeWeightCompensation = lNewCommitDiff / lCurrentWeight;
      rInterface->mCommit = rInterface->mCommit - lRelativeWeightCompensation;
    }
  }

  //For debugging now
  for (auto const &rInterface: lThisGroupsInterfaces) {
    forDebuggingCalculation += rInterface->mCommit;
  }

  LOGGER(false, LOGG_NOTIFY, "Calculation error diff: " << (100.0 - forDebuggingCalculation))

  return EFPBondingMessages::noError;
}

//FIXME - Code is not optimized.
EFPBondingMessages EFPBonding::modifyTotalGroupCommit(std::vector<EFPBonding::EFPInterfaceCommit> &rInterfacesCommit) {

  double lTotalCommit = 0;
  EFPBonding::EFPBondingGroupID lCurrentGroup = 0;

  for (auto const &rThisCommit: rInterfacesCommit) {
    lTotalCommit += rThisCommit.mCommit;
    if (!lCurrentGroup) {
      lCurrentGroup = rThisCommit.mGroupID;
    } else if (rThisCommit.mGroupID != lCurrentGroup) {
      LOGGER(true, LOGG_ERROR, "Not allowed to provide more than one group ID")
      return EFPBondingMessages::parameterError;
    }
  }
  if (lTotalCommit != 100.0 || !lCurrentGroup) {
    return EFPBondingMessages::coverageNot100Percent;
  }

  std::vector<std::shared_ptr<EFPInterface>> lThisGroupsInterfaces;
  for (auto const &rInterfaces: mGroupList) {
    for (auto const &rInterface: rInterfaces) {
      if (rInterface->mGroupID == lCurrentGroup) {
        for (auto const &rNewCommit: rInterfacesCommit) {
          if (rNewCommit.mInterfaceID == rInterface->mInterfaceID) {
            lThisGroupsInterfaces.push_back(rInterface);
            break;
          }
        }
      }
    }
  }

  if (lThisGroupsInterfaces.size() != rInterfacesCommit.size()) {
    LOGGER(true, LOGG_ERROR, "Interface number and setting number does not match")
    return EFPBondingMessages::parameterError;
  }

  //Ok everything seams ok let's push the new settings
  for (auto const &rInterface: lThisGroupsInterfaces) {
    for (auto const &rNewCommit: rInterfacesCommit) {
      if (rNewCommit.mInterfaceID == rInterface->mInterfaceID) {
        rInterface->mCommit = rNewCommit.mCommit;
      }
    }
  }

  return EFPBondingMessages::noError;
}

EFPBonding::EFPStatistics EFPBonding::getStatistics(EFPBonding::EFPBondingInterfaceID interfaceID,
                                                    EFPBonding::EFPBondingGroupID groupID,
                                                    bool reset) {
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
        if (reset) {
          rInterface->mPacketCounter = 0;
          rInterface->mForwardMissingFragment = 0;
        }
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

EFPBonding::EFPBondingGroupID EFPBonding::addInterfaceGroup(std::vector<EFPInterface> &rInterfaces) {
  if (!rInterfaces.size()) { //We need at least a interface
    return 0;
  }

  //Auto assign offset and coverage
  double lCommit = 100.0 / (double) rInterfaces.size();
  double lOffset = 0.0;
  bool didProvideMasterInterface = false;
  std::vector<std::shared_ptr<EFPInterface>> lGroupList;
  for (auto const &rInterface: rInterfaces) {
    if (rInterface.mMasterInterface) {
      didProvideMasterInterface = true;
    }
    std::shared_ptr<EFPInterface> lThisInterface = std::make_shared<EFPInterface>();
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

//FIXME - add round robin type distribution between the interfaces.
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

