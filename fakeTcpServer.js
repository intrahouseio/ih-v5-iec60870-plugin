/**
 * fakeTcpServer.js
 * Запуск:  node fakeTcpServer <port>
 */

const util = require('util');
const net = require('net');

const port = process.argv[2];
console.log('Port = '+port);
if (!port) {
  console.log('Expected port as argv!');
  process.exit();
}

let total = 5;
let sent = 0;
const server = net.createServer(c => {
  console.log('client connected');
  sent = 0;
  c.write('HELLO');
  c.on('end', () => {
    console.log('client disconnected');
  });

  c.on('data', data => {
    console.log('Get data from client ' + data.toString('hex'));
    c.write('Answer');
    /*
    const buf = Buffer.allocUnsafe(4);
    buf.writeFloatLE();
    */
    /*
   if (total && sent < total) {
     sent ++;
    const buf =  testResp(data.toString());
    c.write(buf);
    console.log('Sent ' + buf.toString());
   } else {
    console.log('Test timeout. Total= ' + total);
   }
   */
  });

 
});
server.on('error', err => {
  console.log('ERROR: ' + util.inspect(err));
});

server.listen(port, () => {
  console.log('Server bound on port ' + port);
});

// function sendNext()

/*
function testResp(req) {
  let buf;
  switch (req) {
    case 'W8':
      buf = Buffer.allocUnsafe(1);
      buf.writeInt8(42);
      return buf;
    case 'W16':
      buf = Buffer.allocUnsafe(2);
      buf.writeInt16LE(420);
      return buf;

    case 'W32':
      buf = Buffer.allocUnsafe(4);
      buf.writeInt32LE(42000);
      return buf;
    default:
      buf = Buffer.allocUnsafe(4);
      buf.writeFloatLE(42);
      return buf;
  }
  */
