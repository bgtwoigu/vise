#
# Dockerfile for creating a docker image that runs the VGG Image Search Engine 
# (VISE).
#
# Author: Abhishek Dutta <adutta@robots.ox.ac.uk>
# Date: 05 June 2017
#

#
# Install Debian as base image (tightly controlled and minimal, 150 MB)
# See: https://hub.docker.com/_/debian/
#
FROM debian:jessie

#
# Define image labels
#
LABEL name="VGG Image Search Engine" codename="VISE" version="1.0.0" maintainer="Abhishek Dutta <adutta@robots.ox.ac.uk>" description="VGG Image Search Engine (VISE) enables image instance based search of a large collection of images."

#
# Define ports used by VISE
#   VISE trainer/loader : 9971
#   VISE loaded engine frontend : 9973
#
EXPOSE 9971 9973

#
# Define environment variables
#
ENV VGG_ROOT_DIR="/opt/ox/vgg/" VISE_ROOT_DIR="/opt/ox/vgg/vise/" VISE_CODE_DIR="/opt/ox/vgg/vise/vise-1.0.0/" VISE_DEP_DIR="/opt/ox/vgg/vise/build_deps/" VISE_DEP_LIBDIR="/opt/ox/vgg/vise/build_deps/lib/" VISE_DEP_SRCDIR="/opt/ox/vgg/vise/build_deps/tmp_libsrc/" VISE_SERVER_PORT="9971" VISE_SERVER_NUMTHREADS="4"

#ENV VGG_ROOT_DIR="~/vgg/" VISE_ROOT_DIR="~/vgg/vise/" VISE_CODE_DIR="~/vgg/vise/vise-1.0.0-beta/" VISE_DATA_DIR="~/vgg/vise/vise_data/" VISE_DEP_DIR="~/vgg/vise/build_deps/" VISE_DEP_LIBDIR="~/vgg/vise/build_deps/lib/" VISE_DEP_SRCDIR="~/vgg/vise/build_deps/tmp_libsrc/" VISE_SERVER_PORT="9971" VISE_SERVER_NUMTHREADS="4"

#
# Prepare environment variables
#
RUN mkdir -p $VGG_ROOT_DIR $VISE_ROOT_DIR $VISE_CODE_DIR $VISE_DATA_DIR $VISE_DEP_DIR $VISE_DEP_SRCDIR $VISE_DEP_LIBDIR

#
# Install VISE dependencies
# 
RUN apt-get update && apt-get install -y curl libpng12-dev libjpeg-dev cmake ssh openmpi-bin libopenmpi-dev python python-dev python-pip unzip libmagick++-dev libprotobuf-dev protobuf-compiler libboost-atomic-dev libboost-chrono-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev libboost-timer-dev && pip install cherrypy numpy pillow cython

#
# Install fastann
#
RUN /bin/bash -c 'export FASTANN_LIBDIR=$VISE_DEP_LIBDIR"fastann/" ; \
 cd $VISE_DEP_SRCDIR ; \
 curl -o fastann.zip https://codeload.github.com/philbinj/fastann/zip/master ; \
 unzip fastann.zip ; \
 mv fastann-master fastann ; \
 cd fastann ; \
 mkdir build ; \
 cd build ; \
 export PREFIX=$FASTANN_LIBDIR ; \
 cmake ../ ; make -j8 ; make install'

#
# Download VISE source : http://vgg.gitlab.io/vise/release/vise-1.0.0-beta.tar.gz
#
RUN /bin/bash -c 'export FASTANN_LIBDIR=$VISE_DEP_LIBDIR"fastann/" ; \
# cd $VISE_CODE_DIR ; \
# curl -o vise-1.0.0.tar.gz http://vgg.gitlab.io/vise/release/vise-1.0.0.tar.gz ; \
# tar -zxf vise-1.0.0.tar.gz ; \
# mkdir build ; \
#
# Compile pypar and dkmeans
#
 cd $VISE_CODE_DIR"src/external/dkmeans_relja/pypar_2.1.4_94/source" ; \
 python setup.py build ; \
 python setup.py install ; \
 cd $VISE_CODE_DIR"src/external/dkmeans_relja/" ; \
 python setup.py build ; \
# python setup.py install ; \
 python setup.py install'

#
# Compile VISE
#
# cd $VISE_CODE_DIR"build/" ; \
# cmake -DCMAKE_PREFIX_PATH=$FASTANN_LIBDIR ../src ; \
# make -j8'

#ENTRYPOINT ["$VISE_CODE_DIR'build/vise'", "$VISE_CODE_DIR", "$VISE_DATA_DIR"] # does not expant environment variables
#ENTRYPOINT $VISE_CODE_DIR"build/vise" $VISE_SERVER_PORT $VISE_SERVER_NUMTHREADS $VISE_CODE_DIR /home/$USER/vgg/vise/
#ENTRYPOINT $VISE_CODE_DIR"build/vise" $VISE_SERVER_PORT $VISE_SERVER_NUMTHREADS $VISE_CODE_DIR /home/$USER/vgg/vise/

