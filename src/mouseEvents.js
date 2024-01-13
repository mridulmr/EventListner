const EventEmitter = require("events");

const MouseEvents = (cProgram) => {
  cProgram.stdout.setMaxListeners(20); // Increase the limit for stdout listeners
  cProgram.stderr.setMaxListeners(20); // Increase the limit for stderr listeners
  cProgram.setMaxListeners(20); // Increase the limit for exit listeners

  let capturedData = {
    status: false,
    message: '',
    data: null
  };

  // Handle data received from the C program
  const handleData = (data) => {
    const eventData = data.toString().trim();
    const keyCode = eventData.replace("KEYPRESS:", "").trim();

    capturedData = {
      status: true,
      message: "Key Pressed",
      data: keyCode
    };

    // Emit an event with the captured data
    mouseEventsEmitter.emit("dataReceived", capturedData);
  };

  // Handle errors from the C program
  const handleError = (data) => {
    console.error("Error:", data.toString());
    capturedData = {
      status: false,
      message: data.toString(),
      data: null
    };

    // Emit an event with the captured data
    mouseEventsEmitter.emit("dataReceived", capturedData);
  };

  // Handle the C program exit
  const handleExit = (code) => {
    console.log("C program exited with code", code);
    capturedData.message = capturedData.data ? capturedData.data.toString() : "No data captured";
    capturedData.data = null;

    // Emit an event with the captured data
    mouseEventsEmitter.emit("dataReceived", capturedData);
  };

  cProgram.stdout.on("data", handleData);
  cProgram.stderr.on("data", handleError);
  cProgram.on("close", handleExit);
};

// Create an EventEmitter instance for MouseEvents
const mouseEventsEmitter = new EventEmitter();

module.exports = { MouseEvents, mouseEventsEmitter };
