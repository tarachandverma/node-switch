var http = require('http');
var switches = require('../');


switches.initialize("./conf/libswitch.conf", function(error){
  if(error!=null) {
  	console.log(error);
  } else {
  	console.log('successfully initialized');
  }
});


http.createServer(function (req, res) {

  res.writeHead(200, {'Content-Type': 'text/plain'});
  var isEnabled = switches.isSwitchEnabled("module-1","uuid","dj_chandt");
  res.end('module-1/uuid/dj_chand is set to ' + isEnabled);
  
  console.log(switches.isSwitchEnabled("module-1","uuid","dj_chandt"));
  
}).listen(1337, '127.0.0.1');

console.log('Server running at http://127.0.0.1:1337/');


