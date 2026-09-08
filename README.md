# PRAHARI — Landslide Risk Monitoring System

PRAHARI is an IoT + AI-based landslide risk monitoring system that collects environmental and vibration data using ESP32 sensors, processes it through a backend, and displays the risk level on a web dashboard.

## 📁 Project Structure

```text
PRAHARI/
│
├── backend/
│   ├── data/              # Wayanad datasets
│   ├── model/             # ML model and metrics
│   ├── src/               # Backend source files
│   ├── server.js          # Backend API
│   ├── train_model.py     # ML training
│   ├── sample-payload.json# Sample sensor data
│   ├── package.json
│   └── Dockerfile
│
├── frontend/
│   ├── public/            # Static files
│   ├── src/
│   │   ├── assets/        # Logo and images
│   │   ├── PrahariDashboard.jsx
│   │   └── main.jsx
│   ├── index.html
│   ├── package.json
│   └── vite.config.js
│
├── esp32/
│   └── prahari_esp_v3.ino # ESP32 firmware
│
└── README.md
```

## 🔄 System Flow

```text
ESP32 + Sensors
      ↓
Backend API
      ↓
ML Risk Prediction
      ↓
Web Dashboard
```

### Hardware

ESP32 collects soil moisture, temperature, humidity, pressure, rain and vibration data.

### Backend

Node.js/Express receives sensor data and provides the API for the dashboard.

### ML

The ML model is **currently being trained using Wayanad landslide data**. It is still under development and is **not yet ready to be trained/generalized for additional regions or larger datasets**.

### Frontend

React + Vite displays sensor readings, risk level, charts and system status.

> **Current Status:** Prototype under development. The ML model is currently focused on Wayanad data and requires further training and validation before being extended to other datasets or regions.
