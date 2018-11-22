 /*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const IMG_WIDTH = 640;
const IMG_HEIGHT = 480;
const CANVAS_WIDTH = 640;
const CANVAS_HEIGHT = 480;

window.onload = function(){
    var canvas;
    var frame_timestamp = new Array(100);
    var request_time;
    var frame_number = 0;

    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        var previewElement = document.getElementById('camera-view');

        if (xhr.readyState === 4) {
            update_fps();
            // console.log(JSON.stringify(frame_timestamp));

            var latencyTag = document.getElementById('latency');
            if (latencyTag != null)
                latencyTag.innerHTML = 'latency: ' + (Date.now() - request_time) + ' ms';

            previewElement.src = xhr.response;
        }
    };

    canvas = new Canvas("camera-view-canvas");

    runWebSocket();

    function update_fps() {
        frame_timestamp[frame_number] = Date.now();
        var fpsTag = document.getElementById('fps');
        if (fpsTag != null)
            fpsTag.innerHTML = JSON.stringify(fps());
        if (++frame_number == frame_timestamp.length) frame_number = 0;
    }

    function fps() {
        var i = frame_number;
        var now = Date.now();
        do { // backtrace to find the frame 1000 milliseconds before
            if (now - frame_timestamp[i] >= 1000) {
                var fps = frame_number - i;
                if (fps < 0)
                    fps += frame_timestamp.length;
                return fps;
            }
            if (--i < 0) // wrap around to the last slot in the array
                i = frame_timestamp.length - 1;
        } while (i != frame_number)
    }

    function runWebSocket() {
        // var wsUri = "ws://192.168.1.211:8888/";
        var wsUri = "ws://" + window.location.hostname + ":8888/";

        websocket = new WebSocket(wsUri);
        websocket.onopen = function(evt) { onOpen(evt) };
        websocket.onclose = function(evt) { onClose(evt) };
        websocket.onmessage = function(evt) { onMessage(evt) };
        websocket.onerror = function(evt) { onError(evt) };

        // // polling
        // setInterval(function() {
        //   // console.log('hey!');
        //   xhr.open('POST', '/requestImage' , true);
        //   xhr.send();
        //   request_time = Date.now();
        //   // console.log('update image');
        // }, 100);
    }

    function onOpen(evt)
    {
        writeToScreen("CONNECTED");
        doSend("HELLO FROM BROWSER via WebSocket!!!!!!!!!!!!");
    }

    function onClose(evt)
    {
        writeToScreen("DISCONNECTED");
    }

    function onMessage(evt)
    {
        // writeToScreen('<span style="color: blue;">RESPONSE: ' + evt.data+'</span>');

        var urlCreator = window.URL || window.webkitURL;
        var imageUrl = urlCreator.createObjectURL(evt.data);
        document.querySelector("#camera-view").src = imageUrl;
        update_fps();

        var arrayBuffer;
        var fileReader = new FileReader();
        fileReader.onload = function(event) {
            arrayBuffer = event.target.result;
            var exif = EXIF.readFromBinaryFile(arrayBuffer);
            var exifInfoString = asciiToStr(exif.UserComment, 8);
            var type = 'blur';
            if (getResultType(exifInfoString) != 0) {
                type = 'active';
            }

            var pointArray = getPointArrayFromString(exifInfoString.slice(4));

            if (pointArray.length <= 0) {
                document.querySelector("#mobile-detection").innerHTML = "No<br>Motion";
            } else {
                document.querySelector("#mobile-detection").innerHTML = "Motion<br>Detected";
            }

            canvas.clearPoints();
            canvas.drawPoints(pointArray, type);
        };
        fileReader.readAsArrayBuffer(evt.data);

        doSend("ack", true);

        // websocket.close();

        // var reader = new FileReader();
        // reader.readAsDataURL(evt.data);
        // reader.onloadend = function() {
        //     base64data = reader.result;
        //     var previewElement = document.getElementById('camera-view');
        //     previewElement.src = base64data;
        //     update_fps();
        // }
    }

    function onError(evt)
    {
        writeToScreen('<span style="color: red;">ERROR:</span> ' + evt.data);
    }

    function doSend(message, dont_print_log)
    {
        websocket.send(message);
        if (!dont_print_log)
            writeToScreen("SENT: " + message);
    }

    function writeToScreen(message)
    {
        var output = document.getElementById("output");
        if (output == null)
            return;
        var pre = document.createElement("p");
        pre.style.wordWrap = "break-word";
        pre.innerHTML = message;
        output.appendChild(pre);
    }

    function asciiToStr(asciiArr, start) {
        var string = "";
        var i = start;

        for (; i < asciiArr.length; i++) {
            string += String.fromCharCode(asciiArr[i]);
        }

        return string;
    }

    function getResultType(str) {
        return Number(str.slice(0, 2));
    }

    function getPointArrayFromString(pointStr) {
        var strLen = pointStr.length;
        var i = 0;
        var pointArray = [];

        while (i < strLen) {
            pointArray.push(pointStr.slice(i, i + 8));
            i += 8;
        }

        return pointArray;
    }

    var step = 0
    const total_steps = 8
    setInterval(function() {
        for (var i = 0; i < total_steps; i++) {
            var className = '.step' + (i + 1);
            $('li' + className).removeClass('blink');
            $('.d-text-tizen' + className).removeClass('blink');
            $('.d-text-st' + className).removeClass('blink');
            $('.arrow' + className).removeClass('blink');
            $('.try-it-bullet').removeClass('shake-it');
        }
        var className = '.step' + (step + 1);
        $('li' + className).addClass('blink');
        $('.d-text-tizen' + className).addClass('blink');
        $('.d-text-st' + className).addClass('blink');
        $('.arrow' + className).addClass('blink');
        if (step == 7)
            $('.try-it-bullet').addClass('shake-it');
        step = ++step % total_steps;
    }, 4000);
};

function Canvas(canvasId) {
    this.viewCanvas = document.getElementById(canvasId);
    this.viewContext = this.viewCanvas.getContext("2d");
}

Canvas.prototype.clearCanvas = function() {
    this.viewContext.clearRect(0,0, CANVAS_WIDTH, CANVAS_HEIGHT);
}

Canvas.prototype.clearPoints = function () {
    this.clearCanvas();
}

Canvas.prototype.drawRect = function(x, y, width, height, color) {
    this.viewContext.beginPath();
    this.viewContext.moveTo(x, y);
    this.viewContext.lineTo(x, y + height);
    this.viewContext.lineTo(x + width, y + height);
    this.viewContext.lineTo(x + width, y);
    this.viewContext.closePath();
    this.viewContext.lineWidth = 3;
    this.viewContext.strokeStyle = color;
    this.viewContext.stroke();
}

Canvas.prototype.drawPoints = function (pointArray, type) {
    var i = 0;
    var x, y, w, h;
    var color;

    if (type == 'blur') {
        color = "rgba(115,232,57,0.8)";
    } else {
        color = "rgba(255,0,0, 0.8)";
    }

    for (i = 0; i < pointArray.length; i++) {
        x = IMG_WIDTH / 99 * parseInt(pointArray[i].slice(0,2));
        y = IMG_HEIGHT / 99 * parseInt(pointArray[i].slice(2,4));
        w = IMG_WIDTH / 99 * parseInt(pointArray[i].slice(4,6));
        h = IMG_HEIGHT / 99 * parseInt(pointArray[i].slice(6,8));

        this.drawRect(x, y, w, h, color);
    }
}
