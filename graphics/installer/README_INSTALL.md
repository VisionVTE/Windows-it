# Apple A18 Pro GPU Windows Driver Installer Guide

This directory contains automated installation scripts and configuration templates to package and install the Apple A18 Pro WDDM graphics driver on Windows 10/11 (x64 and ARM64).

---

## 📂 Directory Structure

```
installer/
├── install.bat          # Elevates privileges and launches install.ps1
├── install.ps1          # Core PowerShell driver packaging/signing/installation script
├── uninstall.bat        # Elevates privileges and launches uninstall.ps1
├── uninstall.ps1        # Core PowerShell driver removal and cleanup script
├── setup.iss            # Inno Setup compilation script (GUI Installer)
└── README_INSTALL.md    # This documentation file
```

---

## 🛠️ Prerequisites

Before installing the driver, ensure the following requirements are met:

1. **Operating System**: Windows 10 or 11 (Build 22621 or newer), running on either **x64** (Intel/AMD) or **ARM64** (e.g. running Windows on Apple Silicon via virtualized/hypervisor environments).
2. **Build Outputs**: You must have compiled the driver in Visual Studio 2022 first.
   - The compiled `AppleA18ProDisplay.dll` should be in either `x64\Release` or `ARM64\Release` (depending on target architecture).
3. **WDK (Windows Driver Kit)**: (Recommended for script installation)
   - WDK is needed to run `Inf2Cat.exe` to generate the security catalog file (`AppleA18Pro.cat`).
   - If WDK is not installed, the installer script will attempt to search for an existing pre-built `.cat` file in the project directories.

---

## 🚀 Option 1: Script Installation (For Developers)

This method is recommended for testing during active development. It handles self-signed certificate generation, local trust store configuration, catalog generation, driver signing, and registration.

### Steps:
1. Open the project root folder.
2. Navigate to the `installer/` directory.
3. Right-click `install.bat` and select **Run as Administrator**.
4. The script will:
   - Verify administrative privileges.
   - Check if **Test Signing** is enabled. If disabled, it will enable it (`bcdedit /set testsigning on`) and prompt you to reboot.
   - Generate a self-signed code-signing certificate (`CN=AppleA18ProDriver`).
   - Register the certificate in the **Trusted Root** and **Trusted Publishers** stores.
   - Create the driver package catalog file (`AppleA18Pro.cat`) using WDK's `Inf2Cat.exe`.
   - Sign `AppleA18ProDisplay.dll` and `AppleA18Pro.cat` using the certificate.
   - Register the driver with the Windows Driver Store using `pnputil.exe`.
5. **Reboot** the computer if prompted (required if Test Signing was just enabled).

---

## 🖥️ Option 2: GUI Installation (For End Users)

You can compile a single executable installer wizard (`.exe`) that packages the driver files and scripts into a self-extracting GUI.

### Steps to Compile:
1. Install [Inno Setup 6](https://jrsoftware.org/isinfo.php) (Standard free installer compiler).
2. Open `setup.iss` in the Inno Setup Compiler.
3. Click **Build ➔ Compile** (or press `Ctrl+F9`).
4. This will compile the setup files and output `AppleA18ProDriverSetup.exe` in the `Output/` folder.

### Steps to Run:
1. Copy `AppleA18ProDriverSetup.exe` to the target Windows machine.
2. Double-click to run it.
3. Follow the graphical wizard. The installer will automatically:
   - Extract the driver files to `Program Files\AppleA18ProDriver`.
   - Run the PowerShell commands in the background to sign the driver and register it with `pnputil`.
4. Reboot the system once the wizard finishes (if Test Signing mode needs to be activated).

---

## 🧹 Uninstallation

To remove the driver and clean up the developer certificates from your system:

- **Using scripts**: Run `uninstall.bat` as Administrator.
- **Using Control Panel**: Navigate to **Settings ➔ Apps ➔ Installed Apps**, locate **Apple A18 Pro GPU Driver**, and click **Uninstall**. The uninstaller will run the cleanup script to delete the driver package and remove the self-signed certificates from the Trusted Root/Trusted Publishers stores.

---

## 🔍 Troubleshooting

### 1. Driver fails to load (Yellow Warning Triangle in Device Manager)
- **Reason**: Test signing mode is disabled, or the self-signed certificate wasn't imported.
- **Fix**: Run `bcdedit /set testsigning on` in an elevated Command Prompt and reboot. Ensure the script imported the certificate to both `Root` and `TrustedPublisher` certificate stores.

### 2. Missing Catalog File Error
- **Reason**: `Inf2Cat.exe` was not found during script installation, and no pre-built `AppleA18Pro.cat` was present.
- **Fix**: Install the Windows Driver Kit (WDK) 11.0 so the script can locate `Inf2Cat.exe`, or manually run the build pipeline which compiles and packages the catalog file.

### 3. Execution Policy Blocking Scripts
- **Reason**: PowerShell execution policy prevents running unsigned scripts.
- **Fix**: The `install.bat` wrapper runs with `-ExecutionPolicy Bypass` to prevent this. Ensure you launch the installation via the batch wrapper rather than running the `.ps1` script directly unless your execution policy is configured to allow it.
