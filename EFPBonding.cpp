//
// UnitX Edgeware AB 2020
//

#include "EFPBonding.h"
#include "Bonding.h"

//Constructor
EFPBonding::EFPBonding() {
  LOGGER(true, LOGG_NOTIFY, "EFPBonding constructed")
  mStreamInterfaces = std::vector<std::vector<EFPInterface>>(256);
  mGlobalPacketCounter = 0;
}

//Destructor
EFPBonding::~EFPBonding() {
  LOGGER(true, LOGG_NOTIFY, "EFPBonding destruct")
}

//Change interface commits in a group
EFPBondingMessages EFPBonding::modifyInterfaceCommits(std::vector<EFPBonding::EFPInterfaceCommit> &rInterfacesCommit, EFPBonding::EFPBondingGroupID groupID) {

  EFPGroup *targetGroup = nullptr;
  for (auto &rGroup: mGroupList) {
    if (rGroup.mGroupID == groupID) {
      targetGroup = &rGroup;
    }
  }

  if (targetGroup == nullptr) {
    return EFPBondingMessages::groupNotFound;
  }

  //We found the target group

  //Check validity of the data;
  //We need to iterate over all interfaces before committing anything
  //to make sure all data and all ID's are there and correct
  double totalCommits = 0.0;
  for (auto const &rThisCommit: rInterfacesCommit) {
    if (rThisCommit.mCommit < 0.0) {
      return EFPBondingMessages::parameterError;
    }
    //Is the interface part of the group?
    bool lInterfaceFound = false;
    for (auto const &rInterface: targetGroup->mGroupInterfaces) {
      if (rInterface->mInterfaceID == rThisCommit.mInterfaceID) {
        lInterfaceFound = true;
        break;
      }
    }
    if (!lInterfaceFound) {
      return EFPBondingMessages::interfaceIDNotFound;
    }
    totalCommits += rThisCommit.mCommit;
  }
  if (totalCommits > 100.0) {
    return EFPBondingMessages::sumCommitHigherThan100Percent;
  }
  if (totalCommits == 0.0) {
    return EFPBondingMessages::parameterError;
  }

  //Here we got some commit level and the interfaces are known to us.

  std::vector<std::shared_ptr<EFPInterface>> lNotModifiedInterfaces;
  double totalRestCommits = 0.0;
  //Set the commit for the interfaces we know and save the sum commit of the rest
  for (auto const &rInterface: targetGroup->mGroupInterfaces) {
    bool lThisInterfaceChanged = false;
    for (auto const &rThisCommit: rInterfacesCommit) {
      if (rInterface->mInterfaceID == rThisCommit.mInterfaceID) {
        rInterface->mCommit = rThisCommit.mCommit;
        lThisInterfaceChanged = true;
        break;
      }
    }

    if (!lThisInterfaceChanged) {
      lNotModifiedInterfaces.push_back(rInterface);
      totalRestCommits += rInterface->mCommit;
    }

  }

  if (!lNotModifiedInterfaces.size()) {
    //all interfaces where modified no need to balance out the rest
    return EFPBondingMessages::noError;
  }

  double leftToBalanceOut = 100.0 - totalCommits;

  if (leftToBalanceOut < 0.1 || totalRestCommits < 0.01 ) {
    //we got less than 0.1% to balance out.. Then the master interface takes that load.
    //We just say no error then less than 0.1% payload will end up on the master interface.
    //Or the interfaces that we want to balance
    return EFPBondingMessages::noError;
  }

  double lDiffMultiplicationFactor = leftToBalanceOut / totalRestCommits;

  for (auto const &rInterface: lNotModifiedInterfaces) {
    double newCommit = rInterface->mCommit * lDiffMultiplicationFactor;
    rInterface->mCommit = newCommit;
  }

  return EFPBondingMessages::noError;

}

//Get statistics for a interface
EFPBonding::EFPStatistics EFPBonding::getStatistics(EFPBonding::EFPBondingInterfaceID interfaceID,
                                                    EFPBonding::EFPBondingGroupID groupID,
                                                    bool reset) {
  EFPBonding::EFPStatistics lMyStatistics;

  //Stats is for split interface
  if (interfaceID && !groupID) {
    for (auto &rInterfaces: mStreamInterfaces) {
        for (auto &rInterface: rInterfaces) {
          if (rInterface.mInterfaceID == interfaceID) {
            lMyStatistics.mNoFragmentsSent += rInterface.mFragmentCounter;
            if (reset) {
              rInterface.mFragmentCounter = 0;
            }
          }
        }
    }
    return lMyStatistics;
  }

  if (!interfaceID || !groupID || !mGroupList.size()) {
    return lMyStatistics;
  }
  for (auto const &rGroup: mGroupList) {
    if (rGroup.mGroupID == groupID) {
    for (auto const &rInterface: rGroup.mGroupInterfaces) {
      if (rInterface->mInterfaceID == interfaceID) {
        lMyStatistics.mNoFragmentsSent = rInterface->mFragmentCounter;
        lMyStatistics.mNoGapsCoveredFor = rInterface->mForwardMissingFragment;
        lMyStatistics.mPercentOfTotalTraffic =
            ((double) rInterface->mFragmentCounter / (double) mGlobalPacketCounter) * 100.0;
        if (reset) {
          rInterface->mFragmentCounter = 0;
          rInterface->mForwardMissingFragment = 0;
        }
        return lMyStatistics;
      }
    }
  }
  }
  return lMyStatistics;
}

//Get the global packet counter
uint64_t EFPBonding::getGlobalPacketCounter() {
  return mGlobalPacketCounter;
}

void EFPBonding::increaseGlobalPacketCounter() {
  mGlobalPacketCounter += 1;
}

//Re-set the global packet counter
void EFPBonding::clearGlobalPacketCounter() {
  mGlobalPacketCounter = 0;
}

//Generate unique interfaceID
EFPBonding::EFPBondingInterfaceID EFPBonding::generateInterfaceID() {
  return mUniqueInterfaceID++;
}


EFPBondingMessages EFPBonding::addInterfaceToStreamID(EFPInterface interface, std::vector<uint8_t> &rEFPIDList) {
  for (auto const &rEFPID: rEFPIDList) {
    if (mStreamInterfaces[rEFPID].size()) {
      for (auto const &rInterface: mStreamInterfaces[rEFPID]) {
        if (rInterface.mInterfaceID == interface.mInterfaceID) {
          return EFPBondingMessages::interfaceAlreadyAdded;
        }
      }
    }
    mStreamInterfaces[rEFPID].push_back(interface);
  }
  return EFPBondingMessages::noError;
}

EFPBondingMessages EFPBonding::removeInterfaceFromStreamID(EFPBondingInterfaceID interfaceID, std::vector<uint8_t> &rEFPIDList) {
  for (auto const &rEFPID: rEFPIDList) {
    if (mStreamInterfaces[rEFPID].size()) {
      int lInterfaceIndex = 0;
      for (auto const &rInterface: mStreamInterfaces[rEFPID]) {
        if (rInterface.mInterfaceID == interfaceID) {
          mStreamInterfaces[rEFPID].erase(mStreamInterfaces[rEFPID].begin() + lInterfaceIndex);
        }
        lInterfaceIndex++;
      }
    }
  }
  return EFPBondingMessages::noError;
}

//Add a interface group to EFPBond
EFPBonding::EFPBondingGroupID EFPBonding::addInterfaceGroup(std::vector<EFPInterface> &rInterfaces) {
  if (!rInterfaces.size()) { //We need at least a interface
    return 0;
  }

  //Auto assign offset and coverage
  double lCommit = 100.0 / (double) rInterfaces.size();
  double lOffset = 0.0;
  bool didProvideMasterInterface = false;
  EFPGroup lGroup;
  lGroup.mGroupID = mUniqueGroupID;
  for (auto const &rInterface: rInterfaces) {
    if (rInterface.mMasterInterface) {
      didProvideMasterInterface = true;
    }
    if (rInterface.mInterfaceLocation == nullptr) {
      LOGGER(true, LOGG_ERROR, "Did not provide interface location")
      return 0;
    }
    std::shared_ptr<EFPInterface> lThisInterface = std::make_shared<EFPInterface>();
    lThisInterface->mInterfaceLocation = rInterface.mInterfaceLocation;
    lThisInterface->mCommit = lCommit;
    lThisInterface->mFireCounter = 0.0;
    lThisInterface->mInterfaceID = rInterface.mInterfaceID;
    lThisInterface->mMasterInterface = rInterface.mMasterInterface;
    lThisInterface->mFragmentCounter = 0;
    lThisInterface->mForwardMissingFragment = 0;
    lThisInterface->mForwardMissingFragment = 0;
    lGroup.mGroupInterfaces.push_back(std::move(lThisInterface));
    lOffset += lCommit;
  }
  if (!didProvideMasterInterface) {
    LOGGER(true, LOGG_ERROR, "Did not provide master interface")
    return 0;
  }
  mGroupList.push_back(std::move(lGroup));
  return mUniqueGroupID++;
}

EFPBondingMessages EFPBonding::removeGroup(EFPBondingGroupID groupID) {
  int lGroupIndex = 0;
  for (auto const &rGroup: mGroupList) {
    if (rGroup.mGroupID == groupID) {
      mGroupList.erase(mGroupList.begin() + lGroupIndex);
      return EFPBondingMessages::noError;
    }
    lGroupIndex++;
  }
  return EFPBondingMessages::groupNotFound;
}

EFPBondingMessages EFPBonding::splitData(const std::vector<uint8_t> &rSubPacket, uint8_t fragmentID) {
  bool noFragmentReciever = true;
  if (fragmentID) {
    if (mStreamInterfaces[fragmentID].size()) {
      for (auto &rInterface: mStreamInterfaces[fragmentID]) {
        rInterface.mInterfaceLocation(rSubPacket);
        rInterface.mFragmentCounter++;
        noFragmentReciever = false;
      }
    }
  }

  if (noFragmentReciever) {
    return EFPBondingMessages::fragmentNotSent;
  }
  return EFPBondingMessages::noError;
}

EFPBondingMessages EFPBonding::distributeData(const std::vector<uint8_t> &rSubPacket, uint8_t fragmentID) {
  if (!mGroupList.size()) {
    return EFPBondingMessages::noGroupsFound;
  }

  uint64_t currentPercentage = mMonotonicPacketCounter % 100;
  if (!currentPercentage) {
    for (auto const &rGroup: mGroupList) {
      for (auto const &rInterface: rGroup.mGroupInterfaces) {
        rInterface->mFireCounter += rInterface->mCommit;
      }
    }
  }

  for (auto const &rGroup: mGroupList) {
    std::vector<std::shared_ptr<EFPInterface>> lRoundRobinInterfaces;
    for (auto const &rInterface: rGroup.mGroupInterfaces) {
      if (rInterface->mFireCounter >= 1.0) {
        lRoundRobinInterfaces.push_back(rInterface);
      }
    }

    //This if covers for fractional calculation to integer loops (you can't send 0,113 fragments for example
    //Example 1/3 equals integer 33% * 3interfaces has at 100% sent 99% data in the first revolution.
    if (!lRoundRobinInterfaces.size()) {
      for (auto const &rInterface: rGroup.mGroupInterfaces) {
        if (rInterface->mMasterInterface) {
          rInterface->mInterfaceLocation(rSubPacket);
          rInterface->mFragmentCounter++;
          rInterface->mForwardMissingFragment++;
        }
      }
    } else {
      uint64_t targetInterface = mMonotonicPacketCounter % lRoundRobinInterfaces.size();
      lRoundRobinInterfaces[targetInterface]->mFireCounter--;
      lRoundRobinInterfaces[targetInterface]->mInterfaceLocation(rSubPacket);
      lRoundRobinInterfaces[targetInterface]->mFragmentCounter++;
    }
  }
  mMonotonicPacketCounter++;
  return EFPBondingMessages::noError;
}

