This is a guide to setting up ns-3, NYU's mmWave module, and our own additions.

Assumes the use of Ubuntu 18.04, you're on your own otherwise.

# Setup

## ns-3 Dependencies

`apt-get install gcc g++ python python3 python3-dev`

## Cloning the ns-3 Repository

`mkdir repos`

`cd repos`

`git clone https://gitlab.com/nsnam/ns-3-allinone.git`

## ns-3 Variants

`cd ns-3-allinone/`

You can use the baseline ns3 version, or skip right over to setup NYU's module.

`./download.py -n ns-3.30`

`./build.py`

`cd ns-3.30`

`./waf clean`

`./waf configure --build-profile=debug--enable-examples --enable-tests`

If you want to use mmWave, make sure you are in the ns-3-allinone directory, then:

`git clone https://github.com/nyuwireless-unipd/ns3-mmwave.git`

`cd ns3-mmwave`

`./waf configure --build-profile=debug--enable-examples --enable-tests`

`./waf`

## Running Our Stuff

Taking ns3-mmwave as project root, place the files as follows:

`/src/internet/model`

* network-slices.h
* tcp-bic2.cc
* tcp-bic2.hh
* tcp-cmu.cc
* tcp-cmu.hh

`/scratch`

* tcp_two.cc

Now look at the file `/src/internet/wscript`. Do a search for `ledbat` or `yeah` and you will find a big list of .cc files. Add the two .cc files above to this list.

./waf   # this builds 
./waf --run tcp_two

