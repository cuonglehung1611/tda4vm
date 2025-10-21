FROM cuong_2204:latest
ARG UID
ARG GID
ARG USER_NAME

RUN apt-get update && apt-get install -y \
    wget \
    tar \
    unzip \
    libssl-dev \
    libstdc++6

# TODO: custom more setup here

# Set the working directory and switch to the new user
USER ${USER_NAME}
WORKDIR /home/${USER_NAME}/workspace