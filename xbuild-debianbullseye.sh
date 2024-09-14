#!/bin/bash

# zvonilka: cross-build on Linux for Debian-bullseye

IMAGE_NAME=zvonilka-debianbullseye-builder
CONTAINER_NAME=zvonilka_debianbullseye_build
ARGS=${@@Q}

set -xe

if ! test -d "../zvonilka" ; then
	exit 1
fi

if ! podman container exists $CONTAINER_NAME ; then
	if ! podman image exists $IMAGE_NAME ; then

		# Create builder image
		cat <<EOF | podman build -t $IMAGE_NAME -f - .
FROM debian:bullseye-slim
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
	 bash -c 'cd /src/zvonilka && source ./build_linux.sh'
fi

if ! podman container top $CONTAINER_NAME ; then
	cat >build_linux.sh <<EOF
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
cat >build_linux.sh <<EOF
set -xe

mkdir -p ../phiola/alib3/_linux-amd64
make -j8 opus soxr \
 -C ../phiola/alib3/_linux-amd64 \
 -f ../Makefile \
 -I ..

mkdir -p _linux-amd64
make -j8 \
 -C _linux-amd64 \
 -f ../Makefile \
 ROOT_DIR=../.. \
 $ARGS
EOF

# Build inside the container
podman exec $CONTAINER_NAME \
 bash -c 'cd /src/zvonilka && source ./build_linux.sh'
