https://www.nsnam.org/docs/release/3.30/tutorial/singlehtml/index.html#build-profiles

## Basic Setup

Assumes Ubuntu 18.04, you're on your own otherwise

apt-get install gcc g++ python python3 python3-dev

mkdir repos

cd repos

git clone https://gitlab.com/nsnam/ns-3-allinone.git




cd ns-3-allinone/

./download.py -n ns-3.30

./build.py	# takes forever



cd ns-3.30

./waf clean

./waf configure --build-profile=debug--enable-examples --enable-tests

## This Repo
Clone this into ns3.30/scratch

## Other Tests
./test.py 	# really takes forever

./waf --run hello-simulator

./waf --run <ns3-program> --command-template="%s <args>"



