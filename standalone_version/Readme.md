# Info
This was the first version of the code, use this code to as start point if you want to create your own solution.

# Usage/Compiling
For compiling you need **Visual Studio Code + Platform IO Plugin** for installation instructions see: [https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode)
1. Download this folder
2. Open the folder in Platform IO
3. Create a secrets.h file (see secrets.h.example file)
4. Build
5. Upload to ESP via serial port

# Using OTA
Note: First Upload must be over the Serial Port!
1. Edit platformio.ini and enter the ip address of your ESP.
![](../docs/standalone_version/pio_settings.png)
2. Use Upload/Monitor Option as usual.
![](../docs/standalone_version/vs_buttons.png)

# Example Output
![](../docs/standalone_version/example_output.png)