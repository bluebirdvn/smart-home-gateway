#!/bin/bash
set -e
HOST_UID=$(id -u)
HOST_GUID=$(id -g)

IMAGE_NAME="yocto-builder:22.04"
CONTAINER_NAME="yocto_build_env"


echo "building docker image"
docker build --build-arg UID=${HOST_UID} --build-arg GID=${HOST_GUID} -t ${IMAGE_NAME} .

echo "starting yocto build container"

docker run --rm -it --name ${CONTAINER_NAME} -v "$(pwd)":/workdir ${IMAGE_NAME} /bin/bash -c "./build.sh"

