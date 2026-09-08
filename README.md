# PRAHARI — Landslide Risk Monitoring System

PRAHARI is an IoT + AI-based landslide risk monitoring system that collects environmental and vibration data using ESP32 sensors, processes it through a backend, and displays the risk level on a web dashboard.

## 📁 Project Structure

```text
PRAHARI_IIC-3.0/
│
├── PRAHARI_BACKEND-main/
│   └── PRAHARI_BACKEND-main/
│       ├── data/              # Wayanad datasets
│       ├── model/             # ML model and metrics
│       ├── src/               # Backend source files
│       ├── server.js          # Backend API server
│       ├── train_model.py     # ML model training
│       ├── sample-payload.json# Sample sensor data
│       ├── package.json       # Backend dependencies
│       └── Dockerfile         # Deployment configuration
│
├── PRAHARI_FRONTEND-main/
│   └── PRAHARI_IIC_3.0-main/
│       ├── public/            # Static files
│       ├── src/               # React application
│       │   ├── assests/       # Logo and images
│       │   ├── PrahariDashboard.jsx
│       │   └── main.jsx
│       ├── index.html
│       ├── package.json
│       └── vite.config.js
│
├── prahari_esp_v3/
│   └── prahari_esp_v3.ino    # ESP32 firmware
│
├── render-dashboard.png       # Dashboard screenshot
├── vercel-dashboard.png       # Dashboard screenshot
├── website.png                # Website screenshot
└── README.md
