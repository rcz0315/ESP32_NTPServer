#ifndef HTML_H
#define HTML_H

const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Web Interface</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
        }
        
        .container {
            max-width: 800px;
            margin: auto;
        }
        
        h1 {
            text-align: center;
        }
        
        .button {
            display: block;
            width: 150px;
            padding: 10px;
            margin: 20px auto;
            text-align: center;
            background-color: #4CAF50;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            cursor: pointer;
        }
        
        .button:hover {
            background-color: #45a049;
        }
        
        .time {
            font-family: monospace;
            margin-bottom: 20px;
        }
        
        .nmea {
            font-family: monospace;
            margin-bottom: 20px;
        }
        
        .refresh-checkbox {
            margin: 10px 0;
        }
        
        .file-upload {
            font-family: monospace;
            margin: 20px 0;
        }
        
        .progress {
            width: 100%;
            height: 30px;
            background-color: #dbdbdb;
            border-radius: 5px;
            overflow: hidden;
            margin-top: 10px;
        }
        
        .progress-bar {
            height: 100%;
            background-color: #4CAF50;
            width: 0;
            text-align: center;
            color: white;
            line-height: 30px;
        }
    </style>
</head>

<body>
    <div class="container">
        <h1>ESP32 GPS Status</h1>
        <p style="margin-block-start: -1.5em;font-family: monospace;font-size:15px;text-align:center;">V <span id="versionString"></span></p>
        <div class="refresh-checkbox">
            <label>
                <input type="checkbox" id="autoRefresh"> Auto-refresh every 1 seconds
            </label>
        </div>
        <div class="time">
            <h2>Date Time</h2>
            <pre id="GPStime">Loading time...</pre>
        </div>
        <div class="nmea">
            <h2>NMEA Data</h2>
            <pre id="nmeaData">Loading GPS data...</pre>
        </div>
        <div class="file-upload">
            <h2>OTA Update</h2>
            <form id="uploadForm" action="/update" method="post" enctype="multipart/form-data">
                <input type="file" id="fileInput" name="update" accept=".bin" />
                <button type="button" onclick="uploadFile()">Upload</button>
            </form>
            <div class="progress">
                <div id="progressBar" class="progress-bar">0%</div>
            </div>
        </div>
        <div class="button" onclick="restartESP()">Restart ESP32</div>
    </div>

    <script>
        let autoRefresh = false;
        const refreshInterval = 1000;
        let intervalId;
        
        function fetchData() {
           fetch('/data')
            .then(response => response.json())
            .then(data => {
        document.getElementById('versionString').textContent = data.versionString;
        document.getElementById('GPStime').textContent = data.GPStime;
        document.getElementById('nmeaData').textContent = data.nmeaData;
            })
            .catch(error => console.error('Error fetching data:', error));
        }
        fetchData();
        
        function restartESP() {
            if (confirm('Are you sure you want to restart the ESP32?')) {
                fetch('/restart')
                    .then(response => response.text())
                    .then(text => {
                        alert(text);
                        setTimeout(() => {
                            location.reload();
                        }, 1000);
                    })
                    .catch(error => console.error('Error restarting ESP32:', error));
            }
        }
        
        function updateRefreshStatus() {
            autoRefresh = document.getElementById('autoRefresh').checked;
            if (autoRefresh) {
                intervalId = setInterval(fetchData, refreshInterval);
                fetchData();
            } else {
                clearInterval(intervalId);
            }
        }
        
        function uploadFile() {
            const formData = new FormData(document.getElementById('uploadForm'));
            const fileInput = document.getElementById('fileInput');
            if (fileInput.files.length === 0) {
                alert('Please select a file.');
                return;
            }
        
            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/update', true);
        
            xhr.upload.onprogress = function (e) {
                if (e.lengthComputable) {
                    const percentComplete = Math.round((e.loaded / e.total) * 100);
                    document.getElementById('progressBar').style.width = percentComplete + '%';
                    document.getElementById('progressBar').textContent = percentComplete + '%';
                }
            };
        
            xhr.onload = function () {
                if (xhr.status === 200) {
                    alert('Update successful. Restarting.');
                    setTimeout(() => {
                        location.reload();
                    }, 1000);
                } else {
                    alert('Update failed.');
                }
            };
        
            xhr.onerror = function () {
                alert('Error occurred during file upload.');
            };
        
            xhr.send(formData);
        }
        
        document.getElementById('autoRefresh').addEventListener('change', updateRefreshStatus);
        
        updateRefreshStatus();
    </script>
</body>

</html>
)rawliteral";

#endif
