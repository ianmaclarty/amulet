var status_timer;
var is_in_editor = typeof in_editor !== "undefined";
if (typeof log_output === "undefined") {
    log_output = function(msg) {};
}

function getParameterByName(name) {
    name = name.replace(/[\[]/, "\\\[").replace(/[\]]/, "\\\]");
    var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
        results = regex.exec(location.search);
    return results == null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
}

var pointerLockElementName = null;
if ('pointerLockElement' in document) {
    pointerLockElementName = 'pointerLockElement';
} else if ('mozPointerLockElement' in document) {
    pointerLockElementName = 'mozPointerLockElement';
} else if ('webkitPointerLockElement' in document) {
    pointerLockElementName = 'webkitPointerLockElement';
}

var havePointerLock = pointerLockElementName != null;

navigator.getUserMedia  = navigator.getUserMedia ||
                          navigator.webkitGetUserMedia ||
                          navigator.mozGetUserMedia ||
                          navigator.msGetUserMedia;

window.amulet = {};

function run(script) {
    var request = new XMLHttpRequest();
    request.open("GET", script, true);
    request.onload = function() {
        if (this.status >= 200 && this.status < 400) {
            var data = this.responseText;
            load(data);
        } else {
            console.log("error loading script "+script+": "+this.statusText);
        }
    };
    request.onerror = function() {
        console.log("error loading script "+script);
    };
    request.send();
}

function init_canvas() {
    var canvas = document.getElementById("canvas");
    window.amulet.have_pointer_lock = function() {
        return canvas === document[pointerLockElementName];
    };
    // Pop up an alert when webgl context is lost.
    // TODO: handle webgl context restore event.
    // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
    canvas.addEventListener("webglcontextlost", function(e) {
        alert('WebGL context lost. You will need to reload the page or, failing that, restart your browser).');
        e.preventDefault();
    }, false);
    canvas.addEventListener("contextmenu", function(e) { 
        e.preventDefault();
        return false;
    });
    if (havePointerLock) {
        canvas.addEventListener("click", function() {
            if (window.amulet.pointer_lock_requested) {
                canvas.requestPointerLock =
                    canvas.requestPointerLock ||
                    canvas.mozRequestPointerLock ||
                    canvas.webkitRequestPointerLock;
                canvas.requestPointerLock();
            }
        });
        document.exitPointerLock = document.exitPointerLock || document.mozExitPointerLock || document.webkitExitPointerLock;
    }
}

function set_error_text(text) {
    document.getElementById("status").innerHTML = text;
    document.getElementById("status-box").classList.add("error");
    document.getElementById("status-bar").classList.add("error");
}

function set_progress(perc) {
    document.getElementById("status-bar").style.width = perc + "%";
}

var perc = 1;
var delta = 0.65;
var phase1_limit = 33;
function phase1() {
    perc = Math.min(phase1_limit, perc + delta);
    delta = delta * 0.98;
    set_progress(perc);
    //console.log(perc);
}
function phase2() {
    var prog = window.amulet.load_progress || 0;
    set_progress(prog * ((100-phase1_limit)/100) + phase1_limit);
}
var phase = phase1;
function animate_loading_status() {
    if (phase === phase1 && document.readyState == "complete") {
        phase = phase2;
        set_progress(phase1_limit);
    }
    phase();
}

status_timer = setInterval(animate_loading_status, 100);
function clear_status_timer() {
    clearInterval(status_timer);
    status_timer = null;
}

function remove_status_overlay() {
    document.getElementById("status-overlay").style.display = 'none';
    clear_status_timer();
}

var pending_load = null;
var started = false;

function load(script) {
    if (!started) {
        pending_load = script;
    } else {
        Module.ccall('am_emscripten_run', null, ['string'], [script]);
    }
}

function load_gist(id) {
    console.log("loading gist " + id);
    var url = 'https://api.github.com/gists/'+id;
    var request = new XMLHttpRequest();
    request.open("GET", url, true);
    request.onload = function() {
        if (this.status >= 200 && this.status < 400) {
            console.log("received gist");
            var data = this.responseText;
            var res = JSON.parse(data);
            var code = res.files["main.lua"].content;
            load(code);
            remove_status_overlay();
        } else {
            console.log("error loading gist "+url+": "+this.statusText);
        }
    };
    request.onerror = function() {
        console.log("error loading gist "+url);
    };
    request.send();
}

window.amulet.ready = function() {
    var gist = getParameterByName("gist");
    try {
        init_canvas();
        if (!gist || is_in_editor) {
            remove_status_overlay();
        }
    } catch (e) {
        clear_status_timer();
        set_error_text('Something went wrong.<br>See JavaScript console for details.');
        throw e;
    }
    started = true;
    if (gist && !is_in_editor) {
        document.getElementById("container").classList.remove("undecorated-container");
        document.getElementById("container").classList.add("decorated-container");
        document.getElementById("footer").style.display = "block";
        document.getElementById("hacklink").href = "http://www.amulet.xyz/editor.html?gist=" + gist;
        pending_load = null;
        load_gist(gist);
    } else if (pending_load) {
        console.log("loading: " + pending_load);
        load(pending_load);
        pending_load = null;
    }
    setup_audio_resume();
}

window.amulet.error = function(msg) {
    set_error_text(msg);
}

function set_visibility_handler() {
    var hidden, visibilityChange; 
    if (typeof document.hidden !== "undefined") {
      hidden = "hidden";
      visibilityChange = "visibilitychange";
    } else if (typeof document.mozHidden !== "undefined") {
      hidden = "mozHidden";
      visibilityChange = "mozvisibilitychange";
    } else if (typeof document.msHidden !== "undefined") {
      hidden = "msHidden";
      visibilityChange = "msvisibilitychange";
    } else if (typeof document.webkitHidden !== "undefined") {
      hidden = "webkitHidden";
      visibilityChange = "webkitvisibilitychange";
    }
    window.amulet.window_hidden = 0;
    window.amulet.window_has_focus = 1;
    document.addEventListener(visibilityChange, function(event) {
        window.amulet.window_hidden = document[hidden] ? 1 : 0;
    }, false);
    window.addEventListener('blur', function(event) {
        window.amulet.window_has_focus = 0;
    }, false);
    window.addEventListener('focus', function(event) {
        window.amulet.window_has_focus = 1;
    }, false);
}
set_visibility_handler();

function setup_audio_resume() {
    // Modern browsers disable audio until the user interacts with the page.
    // See: https://developers.google.com/web/updates/2017/09/autoplay-policy-changes#webaudio
    window.addEventListener('click', enable_audio, { passive: true, once: true });
    window.addEventListener('keydown', enable_audio, { passive: true, once: true });
    window.addEventListener('touch', enable_audio, { passive: true, once: true });
}
function enable_audio() {
    SDL.audioContext.resume().then(() => {
        console.log('Playback resumed successfully');
    });
}

function run_waiting() {
    Module.ccall('am_emscripten_run_waiting', null, [], []);
}
function pause() {
    Module.ccall('am_emscripten_pause', null, [], []);
}
function resume() {
    Module.ccall('am_emscripten_resume', null, [], []);
}

window.amulet.run_waiting = is_in_editor ? 1 : 0;
window.amulet.no_load_data = getParameterByName("gist") ? 1 : 0;

//------------------------------------------------------------------------

var Module = {
  preRun: [],
  postRun: [],
  print: function(text) {
      console.log(text);
      if (text == "ERROR") {
          if (handle_error) handle_error();
      } else {
          log_output(text);
      }
  },
  printErr: function(text) {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.error(text);
    log_output(text);
  },
  canvas: document.getElementById("canvas"),
  keyboardListeningElement: document.getElementById("player-panel") || document,
  setStatus: function(text) {
      console.log(text);
  },
  totalDependencies: 0,
  monitorRunDependencies: function(left) {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
  }
};
Module.setStatus('Downloading...');
window.onerror = function(event) {
  // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
  set_error_text('Something went wrong.<br>See JavaScript console for details.');
  Module.setStatus = function(text) {
    if (text) Module.printErr('[post-exception status] ' + text);
  };
};
