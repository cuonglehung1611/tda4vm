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

# Export model
# RUN python3 -c " \
#     from ultralytics import YOLO; \
#     print('🟢 Loading YOLOv8n...'); \
#     model = YOLO('yolov8n.pt'); \
#     print('🟢 Exporting ONNX...'); \
#     path = model.export(format='onnx', imgsz=640, simplify=True, device='cpu'); \
#     import os; \
#     size_mb = os.path.getsize(path)/1024/1024; \
#     print(f'✅ SUCCESS! {size_mb:.1f}MB: {path}'); \
#    "

# Set the working directory and switch to the new user
USER ${USER_NAME}
