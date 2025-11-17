FROM cuong_2204:latest

# TODO: add something to install here
# Install Python + deps
RUN apt-get update && apt-get install -y \
    python3.9 python3-pip python3-venv \
    wget curl git && \
    rm -rf /var/lib/apt/lists/*

# Install PyTorch + Ultralytics
RUN pip3 install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu
RUN pip3 install ultralytics onnx onnxruntime

ARG USER_NAME

# Set workspace -> NO COPY! Just set directory
WORKDIR /home/${USER_NAME}/workspace/tda4vm

# Export YOLOv8n to ONNX
COPY export_yolo.py .
RUN python3 export_yolo.py

# Set the default shell
CMD ["/bin/bash"]