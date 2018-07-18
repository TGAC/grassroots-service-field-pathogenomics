# Field Pathogenomics service {#pathogenomics_service_guide}

The Field Pathogenomics service is for tracking the geographical spread of pathogens over time. The method uses new gene sequencing technology to assess the population structure of pathogens directly from infected field samples. It then provides a convenient map-based view of the collected data for easy user access.

## Installation

To build this service, you need the [grassroots core](https://github.com/TGAC/grassroots-core) and [grassroots build config](https://github.com/TGAC/grassroots-build-config) installed and configured. 

The files to build the Field Pathogenomics service are in the ```build/<platform>``` directory. 

### Linux

Enter the build directory 

```
cd build/linux
```

then

```
make all
```

and then 

```
make install
```

to install the service into the Grassroots system where it will be available for use immediately.


## Configuration options


