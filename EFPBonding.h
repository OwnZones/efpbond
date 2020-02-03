//
// UnitX Edgeware AB 2020
//

//Methods are NOT thread safe meaning you can't modify the interfaces while calling other methods.

// Prefixes used
// m class member
// p pointer (*)
// r reference (&)
// h part of header
// l local scope

#ifndef EFPBOND__EFPBONDING_H
#define EFPBOND__EFPBONDING_H

#include <vector>

#define EFP_BONDING_MAJOR_VERSION 1
#define EFP_BONDING_MINOR_VERSION 0

#define MASTER_INTERFACE true
#define NORMAL_INTERFACE false

enum class EFPBondingMessages : int16_t {
  removeInterfaceNotFound = -10000, //Interface ID not found when EFPBonding looks for it
  removeInterfaceOutOfRange, //For some reason the interface was causing a oob mesage
  masterInterfaceMissing, //Master interface not configured. Please do that
  masterInterfaceLocationMissing, //Cant find where to call the master interface
  noError = 0,
  coverageNot100Percent //Warning payload under 100%
};

class EFPBonding {
public:

  ///@EFPBondingID unique id of the interface used by the EFPBonding class
  typedef uint64_t EFPBondingID;

  ///EFPStatistics
  ///@noGapsCoveredFor number of fragments this interface has covered for
  ///@noFragmentsSent fragments sent by this interface
  ///@percentOfTotalTraffic % of total traffic sent by EFPBonding has been sent by this interface
  class EFPStatistics {
  public:
    uint64_t mNoGapsCoveredFor = 0;
    uint64_t mNoFragmentsSent = 0;
    double mPercentOfTotalTraffic = 0;
  };

  ///Constructor
  explicit EFPBonding();

  ///Destructor
  virtual ~EFPBonding();

  ///Delete copy and move constructors and assign operators
  EFPBonding(EFPBonding const &) = delete;              // Copy construct
  EFPBonding(EFPBonding &&) = delete;                   // Move construct
  EFPBonding &operator=(EFPBonding const &) = delete;   // Copy assign
  EFPBonding &operator=(EFPBonding &&) = delete;        // Move assign

  ///Return the version of the current implementation
  uint16_t getVersion() { return (EFP_BONDING_MAJOR_VERSION << 8) | EFP_BONDING_MINOR_VERSION; }

  ///Adds a interface to EFPBonding
  ///@interface pointer to the function that handles the data from EFP
  ///@commit % (integer 1% to 100%) that should be sent over this interface
  ///@offset % offset from 0% to 99% see the graphs in the documentation for the illustration of what tis parameter does
  ///@master true == is master interface else is not.
  EFPBondingID addInterface(std::function<void(const std::vector<uint8_t> &)> rInterface, uint64_t commit, uint64_t offset, bool master);

  ///Removes a interface from EFPBonding
  ///@interfaceID the ID of the interface to remove
  EFPBondingMessages removeInterface(EFPBondingID interfaceID);

  ///Distributes the data to all interfaces registerd
  ///@rSubPacket the data to be distributed
  EFPBondingMessages distributeData(const std::vector<uint8_t> &rSubPacket);

  ///Returns the statistics fo a interface
  ///@interfaceID the ID of the interface to get statistics for
  EFPBonding::EFPStatistics getStatistics(EFPBonding::EFPBondingID interfaceID);

  ///Returns the total number of fragments dealt with by EFPBonding
  uint64_t getGlobalPacketCounter();

  ///Current payload coverage
  uint64_t mCurrentCoverage = 0;

private:
  class EFPInterfaceProps {
  public:
    std::function<void(const std::vector<uint8_t> &)> mInterfaceLocation = nullptr;
    uint64_t mCommit = 0;
    EFPBondingID mInterfaceID = 0;
    double mFireCounter = 0;
    bool mMasterInterface = false;
    std::atomic_uint64_t mPacketCounter;
    std::atomic_uint64_t mForwardMissingFragment;
  };

  uint64_t getCoverage();

  std::atomic_uint64_t mGlobalPacketCounter;
  EFPBondingID mUniqueInterfaceID = 1;
  bool mGotMasterInterface = false;
  std::vector<std::unique_ptr<EFPInterfaceProps>> mInterfaceList;

};

#endif //EFPBOND__EFPBONDING_H
