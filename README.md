# CraftPacker

A user-friendly desktop application for bulk downloading Minecraft mods and their dependencies from Modrinth. Simply provide a list of mod names, and CraftPacker handles the rest, saving you time and effort when setting up new modpacks or updating existing ones.
![image](https://github.com/user-attachments/assets/804444fc-2729-431f-9fdd-8f9dba97fcce)

## ✨ Key Features

*   **Bulk Downloading:** Download dozens of mods from a simple text list.
*   **Automatic Dependency Resolution:** Automatically finds, resolves, and downloads all required dependency mods.
*   **Smart Search:** Intelligently searches Modrinth to find the correct mods, even with informal names.
*   **Import from Folder:** Generate a mod list by scanning an existing Minecraft mods folder.
*   **Customizable:** Specify the exact Minecraft version and mod loader (Fabric, Forge, NeoForge, Quilt).
*   **User-Friendly Interface:** A clean and intuitive GUI with download progress indicators.
*   **Light & Dark Modes:** Includes a sleek, modern interface with a toggle for dark mode.
*   **Standalone Executable:** No need to install Python or any libraries to run the application.
*   **web search** if the api search fails it will use duckduckgo search engine to find the mod in cases the mod name is shortend or incorrect for example jei (just enough items)
## 🚀 Getting Started

This is the easiest way to get started and **does not require Python** or any other programs to be installed.

1.  Go to the [**Releases Page**](https://github.com/helloworldx64/CraftPacker/releases) on the right-hand side of this repository.
2.  Download the latest `CraftPacker-vX.X.X.zip` file from the "Assets" section.
3.  **Extract the .zip file** to a permanent location on your computer (like your Desktop or a new folder in Documents).
4.  Open the new `CraftPacker` folder and run **`CraftPacker.exe`** from inside it.

> [!IMPORTANT]
> **Always extract the folder from the .zip file before running the application.**
>
> While you *can* run `CraftPacker.exe` directly from inside the zip file, this is not recommended. It can cause the application to run slowly and may prevent settings or other files from saving correctly. For the best experience, please extract the folder first.
### For Developers

This method is for those who want to run the application from the source code, modify it, or contribute to the project.

1.  **Clone the repository:**
    ```
    git clone https://github.com/helloworldx64/CraftPacker.git
    cd CraftPacker
    ```

2.  **Create and activate a virtual environment:**
    ```
    # For Windows
    python -m venv venv
    .\venv\Scripts\activate
    ```

3.  **Install the required libraries:**
    ```
    pip install -r requirements.txt
    ```
    *(Note: A `requirements.txt` file should contain `requests`, `sv-ttk`, and `pyinstaller`)*

4.  **Run the application:**
    ```
    python craftpacker.py
    ```

## 📖 How to Use

1.  **Configure Settings:**
    *   Enter your desired **Minecraft Version** (e.g., `1.20.1`).
    *   Select the correct **Loader** (e.g., `fabric`).
    *   Choose a **Download To** directory for your mods.

2.  **Provide a Mod List:**
    *   **Option A (Manual):** Type or paste the names of the mods you want into the "Mod List" box, with one mod per line.
    *   **Option B (Import):** Click **"Import from Folder..."** to automatically generate a list of names from an existing mods folder.

3.  **Search for Mods:**
    *   Click the **"Search Mods"** button. The application will query the Modrinth API.
    *   Found mods will appear in the "Available Mods" list.
    *   Unfound mods will appear in the "Not Found" list.

4.  **Download:**
    *   Click **"Download All Available"** to download every mod in the "Available" list and all their dependencies.
    *   Alternatively, select specific mods in the list and click **"Download Selected"**.

### Example: Quality of Life Mod List (Fabric)

To get you started, here is a popular list of client-side mods that improve performance and add useful features. Simply copy this list and paste it into the 'Mod List' box in CraftPacker.
Sodium
Lithium
Iris Shaders
Just Enough Items
Xaero's Minimap
Jade
Traveler's Backpacks
Clumps


![image](https://github.com/user-attachments/assets/9671b0f8-20ec-4dc8-87a3-eb4fed583f2f)

This list will download the following mods and their dependencies:
*   **Performance:** Sodium, Lithium
*   **Graphics:** Iris Shaders (for shader pack support)
*   **Utilities:** Just Enough Items (recipe viewer), Xaero's Minimap (in-game map), Jade (shows what you are looking at), Traveler's Backpacks (wearable storage), Clumps (groups XP orbs together to reduce lag).
## ❤️ Support the Project

If you find CraftPacker helpful and want to support its development, please consider donating. Your support is greatly appreciated!

[![Donate with PayPal](https://raw.githubusercontent.com/stefan-niedermann/paypal-donate-button/master/paypal-donate-button.png)](https://www.paypal.com/donate/?business=4UZWFGSW6C478&no_recurring=0&item_name=Donate+to+helloworldx64&currency_code=USD )

## 🤝 Contributing

Contributions are welcome! If you have suggestions for improvements or find a bug, please feel free to:
*   Open an [issue](https://github.com/helloworldx64/CraftPacker/issues) to report bugs or request features.
*   Fork the repository and submit a pull request with your changes.

## 📜 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

> [!NOTE]
> *   A huge thank you to the [**Modrinth**](https://modrinth.com/) team for their fantastic platform and free, open API.
> *   This application's modern look is powered by the [**sv-ttk**](https://github.com/rdbende/sv-ttk) theme library.


