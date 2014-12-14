# intended to be sourced: . install_build_tools.sh

# gcc 4.9.x is the dflt version on *ubuntu 14.10, and seems to be REAL memory hog
# gcc 4.8.x is sufficient for our needs, and is much less of a memory hog (thus runs faster on <= 2GB VMs)

sudo apt-get install -y gcc-4.8 g++-4.8 libncurses5-dev libpcre3-dev libboost-dev libboost-system-dev libboost-filesystem-dev ctags

# following modify the make rules to use these versions of gcc/g++
export CC=gcc-4.8
export CXX=g++-4.8
