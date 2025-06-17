var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var enteredData = "";
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function toggle(){
    websocket.send('toggle');
}

const sliders = [
    { id: "sliderRed", valueId: "valueRed" },
    { id: "sliderGreen", valueId: "valueGreen" },
    { id: "sliderBlue", valueId: "valueBlue" }
];

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    document.getElementById('distance').innerHTML = event.data; 
   
}


function updateColorPreview() {
    const r = document.getElementById("sliderRed").value;
    const g = document.getElementById("sliderGreen").value;
    const b = document.getElementById("sliderBlue").value;

    // Update color preview box
    document.getElementById("colorPreview").style.backgroundColor = `rgb(${r}, ${g}, ${b})`;

    // Send RGB values via WebSocket in a clear format
    const message = `RGB:${r},${g},${b}`;

    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(message);
        console.log("Sent to ESP32:", message);
    } else {
        console.warn("WebSocket not connected. Message not sent:", message);
    }
}


sliders.forEach(sliderObj => {
    const slider = document.getElementById(sliderObj.id);
    const valueInput = document.getElementById(sliderObj.valueId);

    // Always sync the number field with the slider
    slider.addEventListener("input", () => {
        valueInput.value = slider.value;
    });
});

// Repeatedly update the color preview and send RGB
setInterval(() => {
    // Sync value display every tick in case something changed
    sliders.forEach(sliderObj => {
        const slider = document.getElementById(sliderObj.id);
        const valueInput = document.getElementById(sliderObj.valueId);
        valueInput.value = slider.value;
    });

    updateColorPreview();
}, 100); // every 100ms = 0.1 seconds


updateColorPreview(); // Set initial preview color
