//
// UnitX Edgeware AB 2020
//

//Methods are NOT thread safe meaning you can't modify the interfaces while calling other methods.

#ifndef EFPBOND__EFPBONDING_H
#define EFPBOND__EFPBONDING_H

#include <vector>

#define EFP_BONDING_MAJOR_VERSION 1
#define EFP_BONDING_MINOR_VERSION 0

#define MASTER_INTERFACE true
#define NORMAL_INTERFACE false

enum class EFPBondingMessages : int16_t {
  removeInterfaceNotFound = -10000,
  removeInterfaceOutOfRange,
  masterInterfaceMissing,
  masterInterfaceLocationMissing,
  noError = 0,
  coverageNot100Percent
};

class EFPBonding {
public:

  class EFPStatistics {
  public:
    uint64_t noGapsCoveredFor = 0;
    uint64_t noFragmentsSent = 0;
    double percentOfTotalTraffic = 0;
  };

  ///Constructor
  explicit EFPBonding();

  ///Destructor
  virtual ~EFPBonding();

  ///Return the version of the current implementation
  uint16_t getVersion() { return (EFP_BONDING_MAJOR_VERSION << 8) | EFP_BONDING_MINOR_VERSION; }

  ///Delete copy and move constructors and assign operators
  EFPBonding(EFPBonding const &) = delete;              // Copy construct
  EFPBonding(EFPBonding &&) = delete;                   // Move construct
  EFPBonding &operator=(EFPBonding const &) = delete;   // Copy assign
  EFPBonding &operator=(EFPBonding &&) = delete;        // Move assign

  typedef uint64_t EFPBondingID;
  EFPBondingID addInterface(std::function<void(const std::vector<uint8_t> &)> interface, uint64_t commit, uint64_t offset, bool master);
  EFPBondingMessages removeInterface(EFPBondingID interfaceID);
  EFPBondingMessages distributeData(const std::vector<uint8_t> &rSubPacket);
  EFPBonding::EFPStatistics getStatistics(EFPBonding::EFPBondingID interfaceID);

  uint64_t currentCoverage = 0;

private:
  class EFPInterfaceProps {
  public:
    std::function<void(const std::vector<uint8_t> &)> interfaceLocation = nullptr;
    uint64_t commit = 0;
    EFPBondingID interfaceID = 0;
    double fireCounter = 0;
    bool masterInterface = false;
    std::atomic_uint64_t packetCounter;
    std::atomic_uint64_t forwardMissingFragment;
  };

  uint64_t getCoverage();

  std::atomic_uint64_t globalPacketCounter;
  EFPBondingID uniqueInterfaceID = 1;
  bool gotMasterInterface = false;
  std::vector<std::unique_ptr<EFPInterfaceProps>> interfaceList;

};

#endif //EFPBOND__EFPBONDING_H
