# CraftPacker (C++ Version)

A fast, native desktop application for bulk downloading Minecraft mods and their dependencies from Modrinth. Simply provide a list of mod names, and CraftPacker handles the rest, saving you time and effort when setting up new modpacks or updating existing ones.

This project is a complete rewrite of the original Python version in C++/Qt for maximum performance and a truly standalone experience.


https://github.com/user-attachments/assets/1cb2c2ec-1c82-4622-891c-47620447b0fa

## ✨ Key Features

*   **Native Performance:** Built with C++ and the Qt framework for a fast and responsive experience.
*   **Bulk Downloading:** Download dozens of mods from a simple text list.
*   **Automatic Dependency Resolution:** Automatically finds, resolves, and downloads all required dependency mods.
*   **Smart Search:** Intelligently searches Modrinth to find the correct mods, even with informal names.
*   **Web Search Fallback:** If the API search fails, it uses DuckDuckGo to find the mod page, handling cases like "jei" for "Just Enough Items".
*   **Import from Folder:** Generate a mod list by scanning an existing Minecraft mods folder.
*   **Customizable:** Specify the exact Minecraft version and mod loader (Fabric, Forge, NeoForge, Quilt).
*   **Right-Click Context Menus:** Open a mod's page directly on Modrinth or search for unfound mods on CurseForge.
*   **Truly Standalone:** The final executable runs on Windows with no need for installers or external runtimes like Python or Java.

## 🚀 Getting Started

### For Users (Easy Install)

This is the easiest way to get started and **does not require any programming tools**.

1.  Go to the [**Releases Page**](https://github.com/helloworldx64/CraftPacker/releases) on the right-hand side of this repository.
2.  Download the latest `CraftPacker-vX.X.X.zip` file from the "Assets" section.
3.  **Extract the .zip file** to a permanent location on your computer (like your Desktop or a new folder in Documents).
4.  Open the new `CraftPacker` folder and run **`CraftPacker.exe`**. All required DLLs are included.

> [!IMPORTANT]
> **Always extract the folder from the .zip file before running the application.** Running the `.exe` from inside the zip file will prevent it from finding its required files and it will not start.

### For Developers (Building from Source)

This method is for those who want to build the application from the source code, modify it, or contribute to the project.

#### Prerequisites
*   A C++ Compiler (e.g., MinGW or MSVC)
*   **Qt 6 Framework** (Desktop development version, including Widgets and Network modules)
*   **CMake** (Version 3.16 or higher)

#### Building the Project
1.  **Clone the repository:**
    ```
    git clone https://github.com/helloworldx64/CraftPacker.git
    cd CraftPacker
    ```

2.  **Configure and build with CMake:**
    ```
    # Create a build directory
    cmake -S . -B build

    # Compile the project
    cmake --build build --config Release
    ```

3.  **Run the Executable:**
    The compiled executable will be located in the build directory (e.g., `build/Release/CraftPacker.exe`). Before running, you must gather the necessary Qt DLLs.

4.  **Deploying (Gathering DLLs):**
    To create a portable package, use the Qt Deployment Tool. Open a Qt command prompt (e.g., "Qt 6.5.3 (MinGW)") and run the following commands:
    ```
    # Navigate to the folder containing your compiled .exe
    cd build/Release

    # Run the deployment tool on the executable
    windeployqt CraftPacker.exe
    ```
    This will copy all required Qt DLLs and plugins into the folder, making it ready to be zipped and distributed.

## 📖 How to Use

1.  **Configure Settings:**
    *   Enter your desired **Minecraft Version** (e.g., `1.20.1`).
    *   Select the correct **Loader** (e.g., `fabric`).
    *   Choose a **Download To** directory for your mods.

2.  **Provide a Mod List:**
    *   **Option A (Manual):** Type or paste mod names into the "Mod List" box (one per line).
    *   **Option B (Import):** Click **"Import from Folder..."** to scan an existing mods folder.

3.  **Search and Download:**
    *   Click **"Search Mods"**. Found mods appear on the left, unfound on the right.
    *   Click **"Download All Available"** or select specific mods and click **"Download Selected"**.

## ❤️ Support the Project

If you find CraftPacker helpful, your support is greatly appreciated!

[![Donate with PayPal](https://raw.githubusercontent.com/stefan-niedermann/paypal-donate-button/master/paypal-donate-button.png)](https://www.paypal.com/donate/?business=4UZWFGSW6C478&no_recurring=0&item_name=Donate+to+helloworldx64&currency_code=USD)

## 🤝 Contributing

Contributions are welcome! If you have suggestions or find a bug, please:
*   Open an [issue](https://github.com/helloworldx64/CraftPacker/issues).
*   Fork the repository and submit a pull request.

## 📜 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

*   A huge thank you to the [**Modrinth**](https://modrinth.com/) team for their fantastic platform and free, open API.
*   This application is built with the incredible [**Qt Framework**](https://www.qt.io/).

