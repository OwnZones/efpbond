//
// UnitX Edgeware AB 2020
//

#include "EFPBonding.h"
#include "Bonding.h"

//Constructor
EFPBonding::EFPBonding() {
  LOGGER(true, LOGG_NOTIFY, "EFPBonding constructed")
  globalPacketCounter = 0;
}

//Destructor
EFPBonding::~EFPBonding() {
  LOGGER(true, LOGG_NOTIFY, "EFPBonding destruct")
}

EFPBonding::EFPBondingID EFPBonding::addInterface(std::function<void(const std::vector<uint8_t> &)> interface,
                                                  uint64_t commit,
                                                  uint64_t offset,
                                                  bool master) {
  if (commit > 100 || offset > 99 || (commit + offset) > 100 || commit < 1) {
    return 0;
  }

  if (master) {
    gotMasterInterface = true;
  }
  std::unique_ptr<EFPInterfaceProps> thisInterface = std::make_unique<EFPInterfaceProps>();
  thisInterface->interfaceLocation = interface;
  thisInterface->commit = commit;
  thisInterface->fireCounter = (double)offset / 100.0;
  thisInterface->interfaceID = uniqueInterfaceID;
  thisInterface->masterInterface = master;
  thisInterface->packetCounter = 0;
  thisInterface->forwardMissingFragment = 0;
  interfaceList.push_back(std::move(thisInterface));
  currentCoverage = getCoverage();
  return uniqueInterfaceID++;
}

EFPBondingMessages EFPBonding::removeInterface(EFPBonding::EFPBondingID interfaceID) {
  int interfaceIndex = 0;
  for (auto const &interface: interfaceList) {
    if (interface->interfaceID == interfaceID) {
      if (interface->masterInterface) {
        gotMasterInterface = false;
      }
      try {
        interfaceList.erase(interfaceList.begin() + interfaceIndex);
        currentCoverage = getCoverage();
        return EFPBondingMessages::noError;
      } catch (const std::out_of_range &oor) {
        LOGGER(true, LOGG_NOTIFY, "removeInterface out of range: " << oor.what())
        return EFPBondingMessages::removeInterfaceOutOfRange;
      }
    }
    interfaceIndex++;
  }
  return EFPBondingMessages::removeInterfaceNotFound;
}

uint64_t EFPBonding::getCoverage() {
  uint64_t totalPercent = 0;
  for (auto const &interface: interfaceList) {
    totalPercent += interface->commit;
  }
  return totalPercent;
}



EFPBondingMessages EFPBonding::distributeData(const std::vector<uint8_t> &rSubPacket) {
  if (currentCoverage < 100) return EFPBondingMessages::coverageNot100Percent;
  if (!gotMasterInterface) return EFPBondingMessages::masterInterfaceMissing;
  std::function<void(const std::vector<uint8_t> &)> masterInterfaceLocation = nullptr;
  std::atomic_uint64_t *pForwardMissingFragment = nullptr;
  bool didForwardFragment = false;
  globalPacketCounter++;
  for (auto const &interface: interfaceList) {
    if (interface->masterInterface) {
      masterInterfaceLocation = interface->interfaceLocation;
      pForwardMissingFragment = &interface->forwardMissingFragment;
    }
    interface->fireCounter += (double)interface->commit / 100.0;
    if (interface->fireCounter >= 1.0) {
      didForwardFragment = true;
      interface->interfaceLocation(rSubPacket);
      interface->fireCounter -= 1.0;
      interface->packetCounter++;
      //debug
      if (interface->fireCounter >= 1.0) {
        LOGGER(true, LOGG_NOTIFY, "Crazy... Back to school mr. programmer this is wrong.")
      }
    }
  }

  if (!didForwardFragment) {
    LOGGER(true, LOGG_WARN, "EFPBonding is not configured for optimal performance.")
    if(masterInterfaceLocation == nullptr) {
      LOGGER(true, LOGG_FATAL, "Master interface not specified")
      return EFPBondingMessages::masterInterfaceLocationMissing;
    }
    *pForwardMissingFragment++;
    masterInterfaceLocation(rSubPacket);
  }

  return EFPBondingMessages::noError;
}

EFPBonding::EFPStatistics EFPBonding::getStatistics(EFPBonding::EFPBondingID interfaceID) {
  EFPBonding::EFPStatistics myStatistics;
  for (auto const &interface: interfaceList) {
    if (interface->interfaceID == interfaceID) {
      myStatistics.noFragmentsSent = interface->packetCounter;
      myStatistics.noGapsCoveredFor = interface->forwardMissingFragment;
      myStatistics.percentOfTotalTraffic = ((double)interface->packetCounter / (double)globalPacketCounter) * 100.0;
      return myStatistics;
    }
  }
  return myStatistics;
}