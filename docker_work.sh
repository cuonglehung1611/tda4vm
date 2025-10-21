docker build --build-arg USER_ID=$(id -u) \
             --build-arg GROUP_ID=$(id -g) \
             --build-arg USER_NAME=$(whoami) \
             -t cuong_work .

# Define the ssh public key directory for mounting from host to docker
SSH_PUBLIC_KEY_DIR="/home/$(whoami)/.ssh"

# Define the workspace directory
WORKSPACE_DIR="/home/$(whoami)/workspace"

docker run -it \
   --privileged \
   --network=host \
   --cap-add=ALL \
   --security-opt apparmor=unconfined \
   --security-opt seccomp=unconfined \
   --device /dev/loop-control \
   --volume /dev:/dev \
   -v ${WORKSPACE_DIR}:${WORKSPACE_DIR} \
   -v ${SSH_PUBLIC_KEY_DIR}:${SSH_PUBLIC_KEY_DIR} \
   cuong_work
