const char manager_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <title>%DEVICE_NAME%</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #f7f7f7;
    }

    #submit {
      width: 120px;
    }

    #edit_path {
      width: 250px;
    }

    #delete_path {
      width: 250px;
    }

    #spacer_50 {
      height: 50px;
    }

    #spacer_20 {
      height: 20px;
    }

    table {
      background-color: #dddddd;
      border-collapse: collapse;
      width: 650px;
    }

    td,
    th {
      border: 1px solid #dddddd;
      text-align: left;
      padding: 8px;
    }

    #first_td_th {
      width: 400px;
    }

    tr:nth-child(even) {
      background-color: #ffffff;
    }

    #format_notice {
      color: #ff0000;
    }

    #left_div {
      float: left;
      box-sizing: border-box;
      vertical-align: middle;
      display: inline-block;
    }

    #right_div {
      float: right;
      box-sizing: border-box;
      vertical-align: middle;
      display: inline-block;
    }

    #right_div {
      float: right;
      box-sizing: border-box;
      vertical-align: middle;
      display: inline-block;
    }

    #wrap_div {
      margin: auto;
      text-align: center;
    }
  </style>
  <script>
    function validateFormUpdate() {
      var inputElement = document.getElementById('update');
      var files = inputElement.files;
      if (files.length == 0) {
        alert("File Not Selected");
        return false;
      }
      var value = inputElement.value;
      var dotIndex = value.lastIndexOf(".") + 1;
      var valueExtension = value.substring(dotIndex);
      if (valueExtension != "bin") {
        alert("Not A Firmware File");
        return false;
      }
    }
    function confirmFormat() {
      var text = "REBOOT?";
      if (confirm(text) == true) {
        return true;
      }
      else {
        return false;
      }
    }
  </script>
</head>

<body>
  <center>
    <h2>%DEVICE_NAME%</h2>
    <div id="spacer_20"></div>



    <table>
      <td align="center" valign="top">
        <center>
          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Device INFO</legend>
            <table>
              <th scope="col">WiFi SSID</th>
              <td>%SSID%</td>
              </tr>
              <tr>
                <th scope="col">RSSI</th>
                <td>%RSSI% (%WIFI_QUALITY%)</td>
              </tr>
              <tr>
                <th scope="col">Device IP</th>
                <td>%IP%</td>
              </tr>
              <tr>
                <th scope="col">MAC</th>
                <td>%MAC%</td>
              </tr>
              <tr>
                <th scope="col">TCP/IP Status</th>
                <td>%TCP_STATUS%</td>
              </tr>
              <tr>
                <th scope="col">TCP/IP Server</th>
                <td>%TCP_IP%:%TCP_PORT%</td>
              </tr>
              <tr>
                <th scope="col">Mode</th>
                <td>%MODE%</td>
              </tr>
              <tr>
                <th scope="col">RS232 Baud Rate</th>
                <td>%BAUD%</td>
              </tr>
              <tr>
                <th scope="col">Flash Size</th>
                <td>%FlashSize%</td>
              </tr>
              <tr>
                <th scope="col">SD Size</th>
                <td>%SD_Used% / %SD_Total%</td>
              </tr>
              <tr>
                <th scope="col">Heap Memory</th>
                <td>%Heap%</td>
              </tr>
              <tr>
                <th scope="col">F/W Build Date</th>
                <td>%BUILD_VER%</td>
              </tr>
            </table>
          </fieldset>
          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Scale Parameter</legend>
            <table>
              <th scope="col">NET Weight</th>
              <td>%NET_W%g</td>
              </tr>
              <tr>
                <th scope="col">Callibration Weight</th>
                <td>%CAL_W%g</td>
              </tr>
              <tr>
                <th scope="col">Scale Factor</th>
                <td>%SCALE%</td>
              </tr>
              <tr>
                <th scope="col">Input Timeout</th>
                <td>%TM% seconds</td>
              </tr>
            </table>
          </fieldset>
        </center>
      </td>
      <td align="center" valign="top">
        <center>
          <table>
            <tr>
              <td>
                <center>
                  <fieldset style="width:325px;background-color: #f7f7f7;">
                    <legend>WiFi Setting</legend>
                    <form method="POST" action="/wifi">
                      <p>
                        <input type="text" id="WiFi_SSID" name="WiFi_SSID" placeholder="SSID"><br>
                        <input type="text" id="WiFi_PASS" name="WiFi_PASS" placeholder="Password"><br>
                        <!-- <label for="ip">IP Address</label>
                    <input type="text" id="WiFi_IP" name="WiFi_IP" value="192.168.1.200"><br>
                    <label for="gateway">Gateway Address</label>
                    <input type="text" id="WiFi_Gateway" name="WiFi_Gateway" value="192.168.1.1"><br> -->
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
              <td>
                <center>
                  <fieldset style="width:325px;background-color: #f7f7f7;">
                    <legend>TCP/IP Client Setting</legend>
                    <form method="POST" action="/tcpip">
                      <p>
                        <input type="text" id="TCP_IP" name="TCP_IP" placeholder="Server IP"><br>
                        <input type="text" id="TCP_PORT" name="TCP_PORT" placeholder="Port"><br>
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
            </tr>
            <tr></tr>
            <tr>
              <td>
                <center>
                  <fieldset style="width:325px;height:100px;background-color: #f7f7f7;">
                    <legend>RS232 Baud Rate</legend>
                    <form method="POST" action="/baud">
                      <p>
                        <select name="BAUD">
                          <option value="none" selected>Select Baud Rate</option>
                          <option value="300">300bps</option>
                          <option value="1200">1200bps</option>
                          <option value="2400">2400bps</option>
                          <option value="4800">4800bps</option>
                          <option value="9600">9600bps</option>
                          <option value="19200">19200bps</option>
                          <option value="38400">38400bps</option>
                          <option value="57600">57600bps</option>
                          <option value="115200">115200bps</option>
                        </select>
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
              <td>
                <center>
                  <fieldset style="width:325px;height:100px;background-color: #f7f7f7;">
                    <legend>Mode Select</legend>
                    <form method="POST" action="/mode">
                      <p>
                        <input type='radio' name='MODE' value='RS232' />RS232 Only
                        <input type='radio' name='MODE' value='TCP/IP' />RS232+TCP/IP
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
            </tr>
            <tr></tr>
          </table>

          <div id="spacer_20"></div>

          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Scale Parameter</legend>
            <form method="POST" action="/sc_param" enctype="multipart/form-data">
              <p>
              <table>
                <tr>
                  <td>
                    <center>
                      <select name="PARAM">
                        <option value="none" selected>Scale Parameter</option>
                        <option value="NW">Net Weight</option>
                        <option value="CW">Callibration Weight</option>
                        <option value="TM">Input Timeout</option>
                      </select>
                    </center>
                  </td>
                  <td>
                    <center>
                      <input type="text" id="VALUE" name="VALUE" placeholder="Param Value"><br>
                    </center>
                  </td>
                </tr>
              </table>
              <div id="spacer_10"></div>
              </p>
              <input type="submit" value="Submit">
            </form>
          </fieldset>

          <div id="spacer_20"></div>

          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Firmware Update</legend>
            <div id="spacer_20"></div>
            <form method="POST" action="/update" enctype="multipart/form-data">
              <table>
                <tr>
                  <td id="first_td_th">
                    <input type="file" id="update" name="update">
                  </td>
                  <td>
                    <input type="submit" id="submit" value="Start" onclick="return validateFormUpdate()">
                  </td>
                </tr>
              </table>
            </form>
            <div id="spacer_20"></div>
          </fieldset>

          <div id="spacer_20"></div>

          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Device Reboot</legend>
            <form method="GET" action="/reboot" enctype="multipart/form-data">
              <input type="submit" id="submit" value="REBOOT" onclick="return confirmFormat()">
            </form>
          </fieldset>
        </center>
      </td>

    </table>

  </center>
  <iframe style="display:none" name="self_page"></iframe>
</body>

</html> )rawliteral";

const char ok_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <title>Firmware Update Success</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #f7f7f7;
    }

    #spacer_50 {
      height: 50px;
    }
  </style>
</head>

<body>
  <center>
    <h2>Firmware Update Success</h2>
    <div id="spacer_50"></div>
    <button onclick="window.location.href='/';">Return</button>
  </center>
</body>

</html>
)rawliteral";

const char failed_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <title>Update Fail</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #f7f7f7;
    }

    #spacer_50 {
      height: 50px;
    }
  </style>
</head>

<body>
  <center>
    <h2>Firmware Update Fail</h2>
    <div id="spacer_50"></div>
    <button onclick="window.location.href='/';">Return</button>
  </center>
</body>

</html>
)rawliteral";