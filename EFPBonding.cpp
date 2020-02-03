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

EFPBonding::EFPBondingID EFPBonding::addInterface(std::function<void(const std::vector<uint8_t> &)> rInterface,
                                                  uint64_t commit,
                                                  uint64_t offset,
                                                  bool master) {
  if (commit > 100 || offset > 99 || (commit + offset) > 100 || commit < 1) {
    return 0;
  }

  if (master) {
    mGotMasterInterface = true;
  }
  std::unique_ptr<EFPInterfaceProps> lThisInterface = std::make_unique<EFPInterfaceProps>();
  lThisInterface->mInterfaceLocation = rInterface;
  lThisInterface->mCommit = commit;
  lThisInterface->mFireCounter = (double)offset / 100.0;
  lThisInterface->mInterfaceID = mUniqueInterfaceID;
  lThisInterface->mMasterInterface = master;
  lThisInterface->mPacketCounter = 0;
  lThisInterface->mForwardMissingFragment = 0;
  mInterfaceList.push_back(std::move(lThisInterface));
  mCurrentCoverage = getCoverage();
  return mUniqueInterfaceID++;
}

EFPBondingMessages EFPBonding::removeInterface(EFPBonding::EFPBondingID interfaceID) {
  int lInterfaceIndex = 0;
  for (auto const &interface: mInterfaceList) {
    if (interface->mInterfaceID == interfaceID) {
      if (interface->mMasterInterface) {
        mGotMasterInterface = false;
      }
      try {
        mInterfaceList.erase(mInterfaceList.begin() + lInterfaceIndex);
        mCurrentCoverage = getCoverage();
        return EFPBondingMessages::noError;
      } catch (const std::out_of_range &oor) {
        LOGGER(true, LOGG_NOTIFY, "removeInterface out of range: " << oor.what())
        return EFPBondingMessages::removeInterfaceOutOfRange;
      }
    }
    lInterfaceIndex++;
  }
  return EFPBondingMessages::removeInterfaceNotFound;
}

uint64_t EFPBonding::getCoverage() {
  uint64_t lTotalPercent = 0;
  for (auto const &interface: mInterfaceList) {
    lTotalPercent += interface->mCommit;
  }
  return lTotalPercent;
}

EFPBondingMessages EFPBonding::distributeData(const std::vector<uint8_t> &rSubPacket) {
  if (mCurrentCoverage < 100) return EFPBondingMessages::coverageNot100Percent;
  if (!mGotMasterInterface) return EFPBondingMessages::masterInterfaceMissing;
  std::function<void(const std::vector<uint8_t> &)> lMasterInterfaceLocation = nullptr;
  std::atomic_uint64_t *pForwardMissingFragment = nullptr;
  bool lDidForwardFragment = false;
  mGlobalPacketCounter++;
  for (auto const &rInterface: mInterfaceList) {
    if (rInterface->mMasterInterface) {
      lMasterInterfaceLocation = rInterface->mInterfaceLocation;
      pForwardMissingFragment = &rInterface->mForwardMissingFragment;
    }
    rInterface->mFireCounter += (double)rInterface->mCommit / 100.0;
    if (rInterface->mFireCounter >= 1.0) {
      lDidForwardFragment = true;
      rInterface->mInterfaceLocation(rSubPacket);
      rInterface->mFireCounter -= 1.0;
      rInterface->mPacketCounter++;
    }
  }

  if (!lDidForwardFragment) {
    LOGGER(true, LOGG_WARN, "EFPBonding is not configured for optimal performance.")
    if(lMasterInterfaceLocation == nullptr) {
      LOGGER(true, LOGG_FATAL, "Master interface not specified")
      return EFPBondingMessages::masterInterfaceLocationMissing;
    }
    pForwardMissingFragment[0]++;
    lMasterInterfaceLocation(rSubPacket);
  }
  return EFPBondingMessages::noError;
}

EFPBonding::EFPStatistics EFPBonding::getStatistics(EFPBonding::EFPBondingID interfaceID) {
  EFPBonding::EFPStatistics lMyStatistics;
  for (auto const &rInterface: mInterfaceList) {
    if (rInterface->mInterfaceID == interfaceID) {
      lMyStatistics.mNoFragmentsSent = rInterface->mPacketCounter;
      lMyStatistics.mNoGapsCoveredFor = rInterface->mForwardMissingFragment;
      lMyStatistics.mPercentOfTotalTraffic = ((double)rInterface->mPacketCounter / (double)mGlobalPacketCounter) * 100.0;
      return lMyStatistics;
    }
  }
  return lMyStatistics;
}

uint64_t EFPBonding::getGlobalPacketCounter() {
  return mGlobalPacketCounter;
}

