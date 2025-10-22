FROM cuong_2204:latest
ARG UID
ARG GID
ARG USER_NAME

WORKDIR /home/${USER_NAME}/workspace

# Cài Python + Ultralytics
# RUN pip3 install --no-cache-dir ultralytics==8.2.48 onnx onnxruntime

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
    "

# Set the working directory and switch to the new user
USER ${USER_NAME}