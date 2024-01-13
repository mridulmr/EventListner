const EventEmitter = require("events");

const CaptureScreen = (cProgram) => {
  cProgram.stdout.setMaxListeners(20); // Increase the limit for stdout listeners
  cProgram.stderr.setMaxListeners(20); // Increase the limit for stderr listeners
  cProgram.setMaxListeners(20); // Increase the limit for exit listeners

  let capturedData = {
    status: false,
    data: null,
  };

  let fileData = Buffer.from([]);

  const handleData = (data) => {
    fileData = Buffer.concat([fileData, data]);

    capturedData = {
      status: true,
      data: fileData,
    };

    // Emit an event with the captured data
    screenEventsEmitter.emit("dataReceived", capturedData);
  };

  // Handle errors from the C program
  const handleError = (data) => {
    console.error("Error:", data.toString());
    capturedData = {
      status: false,
      message: data.toString(),
      data: null,
    };

    // Emit an event with the captured data
    screenEventsEmitter.emit("dataReceived", capturedData);
  };

  // Handle the C program exit
  const handleExit = (code) => {
    console.log("C program exited with code", code);
    capturedData.message = capturedData.data
      ? capturedData.data.toString()
      : "No data captured";
    capturedData.data = null;

    // Emit an event with the captured data
    screenEventsEmitter.emit("dataReceived", capturedData);
  };

  cProgram.stdout.on("data", handleData);
  cProgram.stderr.on("data", handleError);
  cProgram.on("close", handleExit);
};

// Create an EventEmitter instance for MouseEvents
const screenEventsEmitter = new EventEmitter();

module.exports = { CaptureScreen, screenEventsEmitter };
