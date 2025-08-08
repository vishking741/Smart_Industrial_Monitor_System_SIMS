# AUG-9 (day-1);
started testing on ESP32 in WOKWI 

learnt about gateways in wokwi how to connect to real internet
public can only acces upto the web but private can make data store locally also 

using public sent static data from esp32 in wokwi to webhook.site(random URL gen) the data is sent in format of JSON

data is being recieved at webhook.site and for retrieving data backend is used node.js (localhost:3000) using my-token at webhook.site -> this stores data so that any other backend can recieve it.

from node.js the data is showed at (localhost:8080) , learnt abt CORS(cross origin  resource sharing) some sort of security sys of browser that blocks using data of other servers , even though we are on same localhost ports are different so cors comes into action, for enebling it 

const cors = require("cors");
app.use(cors());

/// achieved -> understood basic communication between ESP32 and web and Localserver 

