const dgram = require('dgram');

const server = dgram.createSocket("udp4");
server.bind(9998);

server.on('message', (message) => {
    const entry = `${Date.now().toString()};${message}`;
    console.log(entry);
});
