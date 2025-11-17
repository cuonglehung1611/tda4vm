#!/usr/bin/env python3
from ultralytics import YOLO
import os

print("🚀 Exporting YOLOv8n to ONNX...")

# Load model
model = YOLO('yolov8n.pt')

# Export to ONNX (optimized for embedded)
model.export(
    format='onnx',
    imgsz=640,
    simplify=True,      # Simplify graph
    opset=12,           # TDA4VM compatible
    dynamic=False       # Fixed input size
)

print("✅ YOLOv8n.onnx exported successfully!")
print(f"Model size: {os.path.getsize('yolov8n.onnx') / 1024 / 1024:.1f} MB")

# Verify
import onnxruntime as ort
ort_session = ort.InferenceSession('yolov8n.onnx')
print("✅ ONNX model verified!")