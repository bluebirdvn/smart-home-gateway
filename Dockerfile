FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Ho_Chi_Minh

RUN apt-get update && apt-get install -y \
    file \
    gawk wget git diffstat unzip texinfo gcc build-essential chrpath \
    socat cpio python3 python3-pip python3-pexpect xz-utils debianutils \
    iputils-ping python3-git python3-jinja2 libegl1-mesa libsdl1.2-dev \
    pylint xterm python3-subunit mesa-common-dev zstd liblz4-tool \
    curl nano sudo locales && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

ARG UID=1000
ARG GID=1000
ENV USERNAME=yoctouser

RUN groupadd -g ${GID} ${USERNAME} && \
    useradd -u ${UID} -g ${GID} -m -s /bin/bash ${USERNAME} && \
    echo "${USERNAME} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${USERNAME} && \
    chmod 0440 /etc/sudoers.d/${USERNAME}

USER ${USERNAME}
WORKDIR /workdir