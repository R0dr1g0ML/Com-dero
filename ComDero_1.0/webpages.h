#pragma once

const char MAIN_page[] PROGMEM = R"rawliteral(
<html>
<head>
  <title>Reloj ESP32</title>
</head>
<body>
  <h1 id="clock">--:--:--</h1>

  <script>
    async function updateTime() {
      const res = await fetch('/api/time');
      const data = await res.json();
      document.getElementById('clock').innerText = data.time;
    }
    setInterval(updateTime, 1000);
    updateTime();
  </script>
</body>
</html>
)rawliteral";



const char CONFIG_page[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset='utf-8'>
<title>Config</title>
</head>
<body>

<h2>Configuración WiFi</h2>

<p><b>%STATUS%</b></p>

<form method='POST' action='/save'>

<h3>Hora actual:</h3>
<p><span id='clock'>Cargando...</span></p>

<h3>Modo:</h3>
<label><input type='radio' name='mode' value='AP' %AP_CHECKED%> AP</label>
<label><input type='radio' name='mode' value='STA' %STA_CHECKED%> STA</label>

<h3>Credenciales WiFi</h3>
SSID: <input name='ssid' value='%SSID%'><br>
Password: <input name='password' value='%PASSWORD%'><br>

<h3>IP (STA)</h3>
IP: <input name='ip' value='%IP%'><br>
Gateway: <input name='gateway' value='%GATEWAY%'><br>

<h3>Zona horaria</h3>
<select name='timezone'>
<option value='-5' %TZ_-5%>(UTC-5) Colombia</option>
<option value='-4' %TZ_-4%>(UTC-4)</option>
<option value='-3' %TZ_-3%>(UTC-3)</option>
<option value='0' %TZ_0%>(UTC 0)</option>
<option value='1' %TZ_1%>(UTC+1 España)</option>
</select>

<br><br>
<input type='submit' value='Guardar'>
</form>

<script>
function updateClock(){
  fetch('/time').then(r=>r.text()).then(t=>{
    document.getElementById('clock').innerText = t;
  });
}
setInterval(updateClock, 1000);
updateClock();
</script>

</body>
</html>
)rawliteral";
