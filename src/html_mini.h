#ifndef HTML_H
#define HTML_H

const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>ESP32 OTA & Restart</title>
	<style>
		body {
		  font-family: Arial, sans-serif;
		  padding: 20px;
		  max-width: 400px;
		  margin: auto;
		}
		h1 {
		  text-align: center;
		  margin-bottom: 30px;
		}
		.button {
		  display: block;
		  width: 100%;
		  padding: 12px;
		  margin: 20px 0;
		  text-align: center;
		  background-color: #4CAF50;
		  color: white;
		  border: none;
		  border-radius: 5px;
		  font-size: 16px;
		  cursor: pointer;
		}
		.button:hover {
		  background-color: #45a049;
		}
		.progress {
		  width: 100%;
		  height: 25px;
		  background-color: #ddd;
		  border-radius: 5px;
		  overflow: hidden;
		  margin-top: 10px;
		}
		.progress-bar {
		  height: 100%;
		  width: 0;
		  background-color: #4CAF50;
		  color: white;
		  text-align: center;
		  line-height: 25px;
		  font-weight: bold;
		}
		input[type="file"] {
		  width: 100%;
		  margin-top: 10px;
		}
	</style>
  </head>

  <body>
    <h1>ESP32 OTA & Restart</h1>
    <p>
      <strong>Firmware Version:</strong>
      <span id="versionString">Loading...</span>
    </p>
    <form id="uploadForm">
      <input type="file" id="fileInput" name="update" accept=".bin" />
      <button type="button" class="button" onclick="uploadFile()">Upload Firmware</button></form>
    <div class="progress">
      <div id="progressBar" class="progress-bar">0%</div></div>
    <button class="button" onclick="restartESP()">Restart ESP32</button>
    <script>
      function loadVersion() {
        fetch('/api/data')
          .then(res => res.json())
          .then(data => {
          document.getElementById('versionString').textContent = data.versionString || 'N/A';
        })
        .catch(err => {
          document.getElementById('versionString').textContent = 'Error';
          console.error('Failed to fetch versionString:', err);
        });
      }
      window.addEventListener('load', () => {
        loadVersion();
      });
      function uploadFile() {
        const fileInput = document.getElementById('fileInput');
        if (!fileInput.files.length) {
          alert('Please select a firmware file (.ino.bin).');
          return;
        }
        const file = fileInput.files[0];
        const formData = new FormData();
        formData.append('update', file);

        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/api/update', true);

        xhr.upload.onprogress = function(e) {
          if (e.lengthComputable) {
            const percent = Math.round((e.loaded / e.total) * 100);
            const bar = document.getElementById('progressBar');
            bar.style.width = percent + '%';
            bar.textContent = percent + '%';
          }
        };

        xhr.onload = function() {
          if (xhr.status === 200) {
            alert('OTA Update Success. ESP will restart.' + res.message);
          } else {
            alert('Update failed.');
          }
          resetUploadUI();
        };

        xhr.onerror = function() {
          alert('Upload error.');
          resetUploadUI();
        };

        xhr.send(formData);
      }

      function resetUploadUI() {
        const bar = document.getElementById('progressBar');
        bar.style.width = '0%';
        bar.textContent = '0%';
        document.getElementById('fileInput').value = '';
      }

      function restartESP() {
        if (!confirm('Confirm ESP32 restart?')) return;

        fetch('/api/restart', {
          method: 'POST'
        })
        .then(res => {
          if (res.ok) {
            alert('ESP32 is restarting.');
          } else {
            alert('Restart failed.');
          }
        })
        .catch(err => alert('Restart failed: ' + err));
      }
    </script>
  </body>
</html>
)rawliteral";
#endif
