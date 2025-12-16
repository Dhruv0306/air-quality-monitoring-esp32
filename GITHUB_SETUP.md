# GitHub Setup Instructions

## Prerequisites
- GitHub account
- Git installed on your computer
- Project files ready

## Step 1: Create Repository on GitHub
1. Go to [github.com](https://github.com)
2. Click "New repository" (green button)
3. Repository name: `air-quality-monitoring-esp32`
4. Description: `ESP32-based air quality monitoring system with cloud connectivity`
5. Set to Public
6. Check "Add a README file"
7. Add .gitignore: Arduino
8. Choose license: MIT License
9. Click "Create repository"

## Step 2: Prepare Local Files
```bash
# Navigate to your project directory
cd e:\CAPSTONE

# Initialize git repository
git init

# Add remote origin
git remote add origin https://github.com/YOUR_USERNAME/air-quality-monitoring-esp32.git
```

## Step 3: Create .gitignore File
```bash
# Arduino IDE files
*.hex
*.eep
*.elf
*.map
*.sym
*.lss
*.lst
build/
*.tmp

# OS files
.DS_Store
Thumbs.db

# IDE files
.vscode/
*.swp
*.swo
*~
```

## Step 4: Upload Files
```bash
# Add all files
git add .

# Commit changes
git commit -m "Initial commit: ESP32 air quality monitoring system"

# Push to GitHub
git push -u origin main
```

## Step 5: Repository Structure
```
air-quality-monitoring-esp32/
├── README.md
├── PMS_DHT_2025_01_06.ino
├── .gitignore
├── docs/
│   ├── wiring-diagram.png
│   └── setup-guide.md
└── examples/
    └── config-template.h
```

## Quick Commands
```bash
# Clone repository
git clone https://github.com/YOUR_USERNAME/air-quality-monitoring-esp32.git

# Update repository
git add .
git commit -m "Update description"
git push

# Check status
git status
```

Replace `YOUR_USERNAME` with your actual GitHub username.