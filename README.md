![alt text](https://bitbucket.org/unitxtra/efpbond/raw/9e7b4f0d8a79343bcabf6781eb1af001e83ce786/efpbondingblack.png)

# EFPBond

The EFPBond is acting as a data distributor from ElasticFrameProtocol and the underlying protocols.

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

The bonding plug-in makes it possible to either use multiple interfaces to create a highet total trougput or for protecting the data sending the same data over multiple interfaces.

You can for example send data over a mix of any UDP/SRT/TSP as illustrated above. For example 100% over a wan connection then as backup 50% over two 4G connections (100% total) as backup.

Please read -> [**EFPBond**](https://edgeware-my.sharepoint.com/:p:/g/personal/anders_cedronius_edgeware_tv/ERnSit7j6udBsZOqkQcMLrQBpKmnfdApG3lehRk4zE-qgQ?e=qXzjfX) for more information.


## Installation

Requires cmake version >= **3.10** and **C++11**

**Release:**

```sh
cmake -DCMAKE_BUILD_TYPE=Release
make
```

***Debug:***

```sh
cmake -DCMAKE_BUILD_TYPE=Debug
make
```

Output: 
 
**efpbondtest**

*efpbondtest* (executable) runs trough the unit tests and returns EXIT_SUCESS if all unit tests pass.

See the source code for examples on how to use.


## Usage

The EFPBond is configured by ->

**Step 1**

Create your EFPBonding worker.

```
EFPBonding myEFPBonding;
```

**Step 2**

Add your interfaces.

```
  interfacesID[0] = myEFPBonding.addInterface(std::bind(&networkInterface1,std::placeholders::_1), 50, 0, MASTER_INTERFACE);
  interfacesID[1] = myEFPBonding.addInterface(std::bind(&networkInterface2,std::placeholders::_1), 50, 50, NORMAL_INTERFACE);
  interfacesID[2] = myEFPBonding.addInterface(std::bind(&networkInterface3,std::placeholders::_1), 100, 0, NORMAL_INTERFACE);
  
  //addInterface returns the unique handle for that interface. 0 means failed to add interface.
  
```

**Step 3**

In your send data callback in EFP relay the packet trough the EFPBonding.

```
void sendData(const std::vector<uint8_t> &rSubPacket) {
  myEFPBonding.distributeData(rSubPacket);
}
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