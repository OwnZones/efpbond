![alt text](https://bitbucket.org/unitxtra/efpbond/raw/9e7b4f0d8a79343bcabf6781eb1af001e83ce786/efpbondingblack.png)

# EFPBond

The EFPBond is acting as a data distributor from [ElasticFrameProtocol](https://bitbucket.org/unitxtra/efp/src/master/) and the underlying protocols.

```
---------------------------------------------------------   /\
| Data type L | Data type L | Data type F | Data type Q |  /  \
---------------------------------------------------------   ||
|                   ElasticFrameProtocol                |   ||
---------------------------------------------------------   ||
|                         EFPBond                       |   ||
---------------------------------------------------------   ||
| Network layer: UDP, TCP, SRT, RIST, Zixi, SCTP, aso.  |  \  /
---------------------------------------------------------   \/

```

The bonding plug-in makes it possible to use multiple interfaces to create a higher total trougput (bonding) and for protecting the data sending the same data over multiple interfaces (1+n).

You can for example send data over a mix of any UDP/SRT/TCP as illustrated above.

Example 100% over a WIFI connection, then as backup 50% each over two 4G connections as backup. That would create a 1+1 protected signal bonding two 4G connections as backup to the primary WIFI connection.

Please read -> [**EFPBond**](https://edgeware-my.sharepoint.com/:p:/g/personal/anders_cedronius_edgeware_tv/Efpyixw-TG5KuUupbCKUgfgBM3zNs-_dhM5RbUBjgdrKpw?e=NcBUBW) for more information.


## Installation

Requires cmake version >= **3.10** and **C++14**

**Release:**

```sh
cmake -DCMAKE_BUILD_TYPE=Release .
make
```

***Debug:***

```sh
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```

Output: 

**efpbond.a**

The static EFPBond library 
 
**efpbondtest**

*efpbondtest* (executable) runs trough the unit tests and returns EXIT_SUCESS if all unit tests pass.

See the source code for examples on how to use EFPBond.


## Usage

The EFPBond is configured by ->

**Step 1**

Create your bonding plug-in

```
EFPBonding myEFPBonding;
```

**Step 2**

Configure your interfaces

```
  EFPBonding::EFPInterface if1 = EFPBonding::EFPInterface(
      std::bind(&networkInterface1, std::placeholders::_1),
      interfacesID,
      EFP_MASTER_INTERFACE);
```

**Step 3**

Create a vector of your interfaces and pass them to EFPBond

```
std::vector<EFPBonding::EFPInterface> lInterfaces;
//For all interfaces ->
lInterfaces.push_back(lInterface);

//When done push the interfaces to EFPBonding creating a EFPBonding-group
EFPBonding::EFPBondingGroupID bondingGroupID = myEFPBonding.addInterfaceGroup(lInterfaces);

```

**Step 4**

Pass fragment to all groups and all interfaces by sending the EFP fragments troug the bonding plug-in.

```
void sendData(const std::vector<uint8_t> &rSubPacket, uint8_t fragmentID) {
  myEFPBonding.distributeDataGroup(rSubPacket);
}

```
If split-mode is used 

```
EFPBondingMessages result = myEFPBonding.splitData(rSubPacket, fragmentID);
 if (result == EFPBondingMessages::fragmentNotSent) {
  std::cout << "Fragment not sent.. Do what?" << std::endl;
 }
```

**Step 5**

Changing the commit % can be done in operation. Just hand over the interfaces ID and the new commit level.
Total commit may not be higher than 100%

```
//Create a empty list of commits
std::vector<EFPBonding::EFPInterfaceCommit> myInterfaceCommits;

//Then populate the list with interface ID's and commits
myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(25, groupInterfacesID[0]));
myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(25, groupInterfacesID[1]));
myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(50, groupInterfacesID[2]));

//Then let EFPBond know what you want. Specify the target group
myEFPBonding.modifyInterfaceCommits(myInterfaceCommits, groupID);

```


## Using EFPBonding in your CMake project

Please see directions in ElasticFrameProrocol repository and the CMakeLists.txt in this project


## Contributing

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Make your additions and write a UnitTest testing it/them.
4. Commit your changes: `git commit -am 'Add some feature'`
5. Push to the branch: `git push origin my-new-feature`
6. Submit a pull request :D

## History

When using multiple interfaces to either load balance between the interfaces or when securing your transport using 1 + n you have traditionally been bound to that transport protocols features.  EFPBond decouples the transport protocol from those features and allows you to select any underlying transport protocol still being able to protect and load balance between multiple transport interfaces and /or transport technologies. 

If you for example use WIFI to transport your media from a location but need 4G as backup. The 4G connection might not support the bandwidth and is also delaying the signal from your remote location to the receiver, since you added more delay on the 4G connection to cater for more re-transmissions if using a ARQ based underlying protocol. Then EFPBond can help you send 100% over your WIFI and spread another (1+1) 100% of the payload over for example two 4G connections. 


## Credits

The UnitX team at Edgeware AB

Maintainer: anders.cedronius(at)edgeware.tv



## License

*MIT*

Read *LICENCE.md* for details