String INDEX_HTML = R"(
<!DOCTYPE html>
<html>

<head>
  <meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate' />
  <meta http-equiv='Pragma' content='no-cache' />
  <meta http-equiv='Expires' content='0' />
  <meta name='viewport' content='width=device-width, initial-scale=1'>

  <title>{NAME}</title>

  <style type='text/css'>
  body {
    margin: 0;
    font-family: Arial, Helvetica, sans-serif;
  }
  form {
    border: 3px solid #f1f1f1;
  }
  h1 { 
    display: block;
    font-size: 2em;
    margin-top: 0.67em;
    margin-bottom: 0.67em;
    margin-left: 0;
    margin-right: 0;
    font-weight: bold;
    text-align: center;
  }
  h2 { 
    display: block;
    font-size: 1.5em;
    margin-top: 0.67em;
    margin-bottom: 0.67em;
    margin-left: 0;
    margin-right: 0;
  }
  input[type=text], input[type=password] {
    width: 100%;
    padding: 12px 20px;
    margin: 8px 0;
    display: inline-block;
    border: 1px solid #ccc;
    box-sizing: border-box;
  }

  button {
    background-color: #4CAF50;
    color: white;
    padding: 5px 5px;
    margin: 5px 0;
    border: none;
    cursor: pointer;
    width: 100%;
    font-size: 1.5em;
  }
  button:hover {
    opacity: 0.8;
  }
  .block {
    display: block;
    width: 100%;
    height: 200px;
    border: none;
    background-color: #3498DB;
    color: white;
    padding: 14px 28px;
    font-size: 56px;
    cursor: pointer;
    text-align: center;
  }
  .block:hover {
    background-color: #3445DB;
    color: white;
  }
  .topnav {
    overflow: hidden;
    background-color: #333;
  }
  .topnav a {
    float: left;
    color: #f2f2f2;
    text-align: center;
    padding: 14px 16px;
    text-decoration: none;
    font-size: 17px;
  }
  .topnav a:hover {
    background-color: #ddd;
    color: black;
  }
  .topnav a.active {
    background-color: #4CAF50;
    color: white;
  }
  table {
    width: 100%;
    background-color: #FFFFFF;
    border-collapse: collapse;
    border-width: 2px;
    border-color: #4CAF50;
    border-style: solid;
    color: #000000;
  }

  td, th {
    border-width: 2px;
    border-color: #4CAF50;
    border-style: solid;
    padding: 5px;
  }

  thead {
    background-color: #4CAF50;
  }
  </style>

  <script>
    function wifiSetup() {
      console.log('button wifiUpdate clicked!');
      var ssid = document.getElementById('ssid').value;
      var password = document.getElementById('password').value;
      var data = {ssid:ssid, password:password};
      var xhr = new XMLHttpRequest();
      var url = '/wifiSetup';
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          // Typical action to be performed when the document is ready:
          if(xhr.responseText != null){
            console.log(xhr.responseText);
          }
        }
      };
      xhr.open('POST', url, true);
      xhr.send(JSON.stringify(data));
      setTimeout(() => { window.location.reload(); }, 1000);
    };

    function volumeSetup() {
      console.log('button volumen clicked!');
      var volumen = document.getElementById('volumen').value;
      var data = volumen;
      websocket.send(data);
      setTimeout(() => { window.location.reload(); }, 1000);
    };

    function OgKeyPress() {
      var firstValue = document.getElementById('largo').value;
      var secondValue = document.getElementById('ancho').value;
      var thirdValue = document.getElementById('alto').value;
      var totalVolumen = document.getElementById('volumen');
      totalVolumen.value = firstValue * secondValue * thirdValue;
      console.log(totalVolumen);
    };

    function show(shown, hidden1, hidden2) {
      document.getElementById(shown).style.display='block';
      document.getElementById(hidden1).style.display='none';
      document.getElementById(hidden2).style.display='none';

      console.log('Datito: ', shown);
      if (shown != 'Page3') {
        var xhr = new XMLHttpRequest();
        var url;
        if (shown == 'Page1') url = '/';
        if (shown == 'Page2') url = '/list';
        console.log('url: ', url);
        xhr.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            // Typical action to be performed when the document is ready:
            if(xhr.responseText != null){
              console.log(xhr.responseText);
            }
          }
        };
        xhr.open('POST', url, true);
      }
      return false;
    };

    var wsUri = 'ws://' + window.location.hostname + ':81/';
    var websocket;
    var timeOut;

    function retry(){
      clearTimeout(timeOut);
      timeOut = setTimeout(function(){websocket = null; init();},5000);
    };

    function init() {
      try{
        websocket = new WebSocket(wsUri);
        websocket.onerror = function(evt) {
          console.log('WebSocket Error!!!', evt);
          retry();
        };
        websocket.onopen = function() {
          websocket.send('Conectado  -  ' + new Date());
        };
        websocket.onclose = function(evt) {
          console.log('WebSocket Close!!!', evt);
          retry();
        };
        websocket.onmessage = function(evt) {
          console.log('Servidor (recibe): ', evt.data);
          var obj = JSON.parse(evt.data);
          if (obj.type == 'cycle') {
            document.getElementById('ciclos').innerHTML  = 'Ciclos: ' + obj.ciclos;
            document.getElementById('horas').innerHTML  = 'Horas: ' + obj.usoUVC;
          } else if (obj.type == 'file') {
            var col = [];
            var data_obj = obj.data;
            for (var i = 0; i < data_obj.length; i++) {
              for (var key in data_obj[i]) {
                if (col.indexOf(key) === -1) {
                  col.push(key);
                }
              }
            }
            var table = document.createElement('table');

            for (var i = 0; i < data_obj.length; i++) {
              var tr = table.insertRow(-1);
              for (var j = 0; j < col.length; j++) {
                var tabCell = tr.insertCell(-1);
                if (j == 0) {
                  tabCell.innerHTML = '<a href=\"/download?file=' + data_obj[i][col[j]] + '\">' + data_obj[i][col[j]] + '</a>';
                } else {
                  tabCell.innerHTML = data_obj[i][col[j]];
                }
              }
            }
            
            var th = table.createTHead();
            var thr = th.insertRow(-1);
            for (var i = 0; i < col.length; i++) {
              var thc = document.createElement('th');
              thc.innerHTML = col[i];
              thr.appendChild(thc);
            }
            
            
            var divContainer = document.getElementById('showData');
            divContainer.innerHTML = '';
            divContainer.appendChild(table);
          } 
        };
      } catch (e){
        retry();
      };
    }

    window.addEventListener('load', init, false);
  </script>

</head>

<body>

  <div id='Page1'>
    <div class='topnav'>
      <a class='active' href='#'>Inicio</a>
      <a href='#' onclick="return show('Page2', 'Page1', 'Page3');">Bit&aacute;cora</a>
      <a href='#' onclick="return show('Page3', 'Page1', 'Page2');">Configuraci&oacute;n</a>
    </div>
    <h1>CICLOS REALIZADOS</h1>
    <h2><p id='ciclos'>-</p></h2>
    <h1>HORAS DE USO</h1>
    <h1>TUBO UVC de 10000 Hrs.</h1>
    <h2><p id='horas'>-</p></h2>
  </div>

  <div id='Page2' style='display:none'>
    <div class='topnav'>
      <a href='#' onclick="return show('Page1', 'Page2', 'Page3');">Inicio</a>
      <a class='active' href='#'>Bit&aacute;cora</a>
      <a href='#' onclick="return show('Page3', 'Page1', 'Page2');">Configuraci&oacute;n</a>
    </div>
    <h1>BIT&Aacute;CORA</h1>
    <p id='showData'></p>
  </div>

  <div id='Page3' style='display:none'>
    <div class='topnav'>
      <a href='#' onclick="return show('Page1', 'Page2', 'Page3');">Inicio</a>
      <a href='#' onclick="return show('Page2', 'Page1', 'Page3');">Bit&aacute;cora</a>
      <a class='active' href='#'>Configuraci&oacute;n</a>
    </div>
    <h1>CONFIGURACI&Oacute;N WiFi</h1>
    <form>
      <div class='container'>
        <label for='ssid'><b>SSID</b></label>
        <input type='text' id='ssid' placeholder='SSID' name='uname' required>
        <label for='password'><b>Password</b></label>
        <input type='password' id='password' placeholder='PASSWORD' name='psw' required>
        <button class='primary' id='wifibtn' type='button' onclick='wifiSetup()'>GUARDAR</button>
      </div>
    </form>
    <h1>CONFIGURACI&Oacute;N VOL&Uacute;MEN</h1>
    <form>
      <div class='container'>
        <label for='largo'><b>Largo</b></label>
        <input type='text' id='largo' placeholder='Largo' name='uname' required onKeyPress='OgKeyPress()' onKeyUp='OgKeyPress()'>
        <label for='ancho'><b>Ancho</b></label>
        <input type='text' id='ancho' placeholder='Ancho' name='uname' required onKeyPress='OgKeyPress()' onKeyUp='OgKeyPress()'>
        <label for='alto'><b>Alto</b></label>
        <input type='text' id='alto' placeholder='Alto' name='uname' required onKeyPress='OgKeyPress()' onKeyUp='OgKeyPress()'>
        <label for='volumen'><b>Vol&uacute;men m&sup3;</b></label>
        <input type='text' id='volumen' placeholder='Vol&uacute;men' name='uname' required>
        <button class='primary' id='volumenbtn' type='button' onclick='volumeSetup()'>GUARDAR</button>
      </div>
    </form>
  </div>

</body>

</html>
)";
