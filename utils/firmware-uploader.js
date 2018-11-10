const fs = require("fs");
const net = require("net");

const PAYLOAD_TYPE_REQUEST = 0x04;
const PAYLOAD_TYPE_DESCRIPTION = 0x05;
const PAYLOAD_TYPE_FLOW_START = 0xA0;
const PAYLOAD_TYPE_FLOW_END = 0xA3;
const PAYLOAD_TYPE_CHUNK = 0xA1;
const PAYLOAD_TYPE_CHUNK_ACK = 0xA2;

const CHUNK_MULTIPLIER = 8;
let chunk_size = 0;
let chunk_buffer = null;
let end_flag = false;

console.log(process.argv[2]);

fs.open(process.argv[2], "r", (err, fd) => {
    if (err) {
        console.error(err);
        return;
    }

    fs.fstat(fd, (err, stats) => {
        const fileBytes = stats.size;
        const fileBytesAligned = CHUNK_MULTIPLIER * Math.ceil(fileBytes / CHUNK_MULTIPLIER);

        const server = net.createServer((socket) => {
            console.log("Client connected!");
            end_flag = false;
            let currChunk = 0;
            const sendChunk = (chunk_num) => {
                chunk_buffer.fill(0);
                fs.read(fd, chunk_buffer, 7, chunk_size, null, (err, bytesRead, buff) => {
                    if (err) {
                        console.error(err);
                        return;
                    }

                    let chunk_bytes_to_send = bytesRead;

                    if (bytesRead < chunk_size) {
                        chunk_buffer[7+chunk_bytes_to_send] = 0x00;
                        chunk_bytes_to_send += 1; // null terminator
                        end_flag = true;
                    }

                    chunk_buffer[0] = PAYLOAD_TYPE_CHUNK;
                    chunk_buffer.writeUInt16LE(currChunk, 1);
                    chunk_buffer.writeUInt32LE(chunk_bytes_to_send, 3);

                    socket.write(chunk_buffer, 'ascii', 7+chunk_bytes_to_send);
                    console.log(`Requested sending ${chunk_num}, sent ${currChunk} (bytes ${7+chunk_bytes_to_send})`);
                    currChunk += 1;
                });
            };

            socket.on('data', (buff) => {
                if (Buffer.isBuffer(buff)) {
                    if (buff[0] === PAYLOAD_TYPE_REQUEST) {
                        chunk_size = buff[1]*CHUNK_MULTIPLIER || 256;
                        chunk_buffer = Buffer.alloc(chunk_size+7, 0, 'ascii');
                        const chunk_count = Math.ceil(fileBytesAligned / chunk_size);

                        const program_name = process.argv[2];

                        const desc_buff = Buffer.alloc(5 + program_name.length, 0, 'ascii');
                        desc_buff.write(program_name, 4, program_name.length, 'ascii');
                        desc_buff[0] = PAYLOAD_TYPE_DESCRIPTION;
                        desc_buff.writeUInt16LE(chunk_count, 1);
                        desc_buff[3] = program_name.length+1;
                        desc_buff[4+program_name.length] = 0x00;

                        socket.write(desc_buff);
                    } else if (buff[0] === PAYLOAD_TYPE_FLOW_START) {
                        console.log('Flow start request');
                        // start flow
                        sendChunk(0);
                    } else if (buff[0] === PAYLOAD_TYPE_CHUNK_ACK) {
                        const ack_chunk = buff.readUInt16LE(1);
                        console.log(`Client ACK chunk ${ack_chunk}`);
                        if (end_flag) {
                            socket.write(Buffer.from([PAYLOAD_TYPE_FLOW_END]));
                        } else {
                            sendChunk(ack_chunk+1);
                        }
                    } else {
                        console.log('Unknown packet!', buff);
                    }
                } else {
                    console.log("Some weird string received: ", buff);
                }
            });
            socket.on('close', () => {
                console.log('Client closed connection!');
            });

        });
        server.listen(10001);
    });
});
