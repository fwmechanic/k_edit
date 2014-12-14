# intended to be sourced: . install_build_tools.sh

# gcc 4.9.x is the dflt version on *ubuntu 14.10, and seems to be
#           a major memory hog (I've observed this w/MinGW too)
# gcc 4.8.x is sufficient for our needs, and is much less of a
#           memory hog (thus runs faster on <= 2GB VMs)

sudo apt-get install -y \
exuberant-ctags         \
g++-4.8                 \
gcc-4.8                 \
libboost-dev            \
libboost-filesystem-dev \
libboost-system-dev     \
libncurses5-dev         \
libpcre3-dev            \

# modify the make rules to use these versions of gcc/g++
export CC=gcc-4.8
export CXX=g++-4.8
