const { exec, spawn } = require("child_process");
const EventEmitter = require("events");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { MouseEvents, mouseEventsEmitter } = require("./src/mouseEvents");
// const { CaptureScreen, screenEventsEmitter } = require("./src/captureEvents");
const MouseEmitter = new EventEmitter();
// const ScreenshotEmmiter = new EventEmitter();
const screenshot = require("screenshot-desktop");

const outputDir = "bin";

const compileCProgram = (sourceFile, outputFile, target) => {
  return new Promise((resolve, reject) => {
    const command = getCompileCommand(sourceFile, outputFile, target);
    exec(command, (error, stdout, stderr) => {
      if (error) {
        reject(error);
      } else {
        resolve();
      }
    });
  });
};

const getCompileCommand = (sourceFile, outputFile, target) => {
  switch (target) {
    case "linux32":
      return `gcc -m32 -o ${outputFile} ${sourceFile} -lX11`;
    case "linux64":
      return `gcc -o ${outputFile} ${sourceFile} -lX11`;
    case "win32":
      return `i686-w64-mingw32-gcc -o ${outputFile} ${sourceFile}`;
    case "win64":
      return `x86_64-w64-mingw32-gcc -o ${outputFile} ${sourceFile}`;
    case "macos_arm64":
      return `gcc -arch arm64 -o ${outputFile} ${sourceFile} -framework ApplicationServices -framework CoreFoundation -framework CoreGraphics`;
    case "macos_x86_64":
      return `gcc -arch x86_64 -o ${outputFile} ${sourceFile} -framework ApplicationServices -framework CoreFoundation -framework CoreGraphics`;
    default:
      throw new Error(`Unsupported target: ${target}`);
  }
};

// Create output directory if it doesn't exist
if (!fs.existsSync(outputDir)) {
  fs.mkdirSync(outputDir);
}

// Determine the target platform dynamically
let target;
if (os.platform() === "linux") {
  target = os.arch() === "x64" ? "linux64" : "linux32";
} else if (os.platform() === "win32") {
  target = os.arch() === "x64" ? "win64" : "win32";
} else if (os.platform() === "darwin") {
  target = os.arch() === "arm64" ? "macos_arm64" : "macos_x86_64";
} else {
  throw new Error("Unsupported platform");
}

// Set permissions and run MouseEvents function
function runEvents(target, type) {
  try {
    const eventPressPath = path.join(
      __dirname,
      "bin",
      `event_${type}_${target}`
    );
    setExecutablePermissions(eventPressPath)
      .then(() => {
        console.log(`Executable permissions set for ${eventPressPath}.`);
        const cProgram = spawn(eventPressPath);
        if (type === "press") {
          MouseEvents(cProgram);
        }
        if (type === "screenshot") {
          CaptureScreen(cProgram);
        }
      })
      .catch((error) => {
        console.error("Failed to set executable permissions:", error);
      });
  } catch (error) {
    console.error("Error occurred while running MouseEvents:", error);
  }
}

// Function to set executable permissions
function setExecutablePermissions(filePath) {
  return new Promise((resolve, reject) => {
    const command = spawn("chmod", ["+x", filePath]);

    command.on("error", (error) => {
      console.error(
        `Failed to set executable permissions for ${filePath}:`,
        error
      );
      reject(error);
    });

    command.on("exit", (code) => {
      if (code === 0) {
        console.log(`Executable permissions set for ${filePath}.`);
        resolve();
      } else {
        const errorMessage = `Failed to set executable permissions for ${filePath}. Exit code: ${code}`;
        console.error(errorMessage);
        reject(new Error(errorMessage));
      }
    });
  });
}

const Mouse = () => {
  return new Promise((resolve, reject) => {
    // Example usage
    const sourceFile = path.join(__dirname, "./program/event_press.c");
    // Compile for the determined target
    compileCProgram(sourceFile, `${outputDir}/event_press_${target}`, target)
      .then(() => {
        console.log(`C program compiled for ${target}`);
        runEvents(target, "press");
        mouseEventsEmitter.on("dataReceived", (data) => {
          MouseEmitter.emit("data", data); // Emit the received data through the dataEmitter
          resolve(data); // Resolve the promise with the received data
        });
      })
      .catch((error) => {
        reject(error); // Reject the promise with the error
        console.error(`Failed to compile for ${target}:`, error);
      });
  });
};

// Request necessary permissions before running the C program
switch (os.platform()) {
  case "linux":
    // Assuming the executing user is already a member of the 'input' group
    exec(
      "sudo apt-get install libxrandr-dev libxtst-dev",
      (error, stdout, stderr) => {
        if (error) {
          console.error("Failed to install required packages:", error);
        } else {
          console.log("Required packages installed.");
          Mouse(); // Compile and run the C program
        }
      }
    );
    break;
  case "win32":
    // No additional permissions required on Windows
    Mouse(); // Compile and run the C program
    break;
  case "darwin":
    // Request administrative privileges using 'sudo' command
    exec("sudo -v", (error, stdout, stderr) => {
      if (error) {
        console.error("Failed to obtain administrative privileges:", error);
      } else {
        console.log("Administrative privileges obtained.");
        // Start the script with elevated privileges
        Mouse(); // Compile and run the C program
        // ScreenCapture();
      }
    });
    break;
  default:
    throw new Error("Unsupported platform");
}

// Function to capture a screenshot
const CaptureScreenshot = (path) => {
  return new Promise((resolve, reject) => {
    screenshot({ filename: path })
      .then((imageBuffer) => {
        resolve(imageBuffer);
      })
      .catch((error) => {
        console.error("Failed to capture screenshot:", error);
        reject(error)
      });
  });
};

module.exports = {
  MouseEmitter,
  CaptureScreenshot,
};
