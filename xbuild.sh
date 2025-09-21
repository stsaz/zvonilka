#!/bin/bash

# zvonilka: cross-build on Linux for Linux/AMD64

IMAGE_NAME=zvonilka-debianbw-builder
CONTAINER_NAME=zvonilka_debianBW_build
BUILD_TARGET=linux
ARGS=${@@Q}

if test "$JOBS" == "" ; then
	JOBS=8
fi

set -xe

if ! test -d "../zvonilka" ; then
	exit 1
fi

if ! podman container exists $CONTAINER_NAME ; then
	if ! podman image exists $IMAGE_NAME ; then

		# Create builder image
		cat <<EOF | podman build -t $IMAGE_NAME -f - .
FROM debian:bookworm-slim
RUN apt update && \
 apt install -y \
  make
RUN apt install -y \
 zstd zip unzip bzip2 xz-utils \
 perl \
 cmake \
 patch \
 dos2unix \
 curl
RUN apt install -y \
 autoconf libtool libtool-bin \
 gettext \
 pkg-config
RUN apt install -y \
 gcc g++
RUN apt install -y \
 libasound2-dev libpulse-dev libjack-dev \
 libdbus-1-dev
EOF
	fi

	# Create builder container
	podman create --attach --tty \
	 -v `pwd`/..:/src \
	 --name $CONTAINER_NAME \
	 $IMAGE_NAME \
	 bash -c "cd /src/zvonilka && source ./build_$BUILD_TARGET.sh"
fi

if ! podman container top $CONTAINER_NAME ; then
	cat >build_$BUILD_TARGET.sh <<EOF
sleep 600
EOF
	# Start container in background
	podman start --attach $CONTAINER_NAME &
	sleep .5
	while ! podman container top $CONTAINER_NAME ; do
		sleep .5
	done
fi

# Prepare build script

ODIR=_linux-amd64

cat >build_$BUILD_TARGET.sh <<EOF
set -xe

mkdir -p ../phiola/alib3/$ODIR
make -j$JOBS opus soxr \
 -C ../phiola/alib3/$ODIR \
 -f ../Makefile \
 -I ..

mkdir -p $ODIR
make -j$JOBS \
 -C $ODIR \
 -f ../Makefile \
 ROOT_DIR=../.. \
 $ARGS
EOF

# Build inside the container
podman exec $CONTAINER_NAME \
 bash -c "cd /src/zvonilka && source ./build_$BUILD_TARGET.sh"
