function load_text(url, cb) {
    var oReq = new XMLHttpRequest();
    oReq.open("GET", url, true);
    oReq.responseType = "text";
    oReq.send();

    oReq.onload = function(oEvent) {
      var txt = oReq.responseText;
      cb(txt);
    };
}

function load_arraybuffer(url, cb) {
    var oReq = new XMLHttpRequest();
    oReq.open("GET", url, true);
    oReq.responseType = "arraybuffer";
    oReq.send();

    oReq.onload = function(oEvent) {
      var arrayBuffer = oReq.response;
      cb(arrayBuffer);
    };
}

function all_loaded(files) {
    return files["amulet.js"] &&
        files["amulet.wasm"] &&
        files["amulet_license.txt"] &&
        files["player.html"];
}

function load_text_file(file, files, on_all_loaded) {
    load_text(file, function(txt) {
        console.log("loaded " + file);
        files[file] = txt;
        if (all_loaded(files)) {
            on_all_loaded(files);
        } else {
            console.log("still more to go...");
        }
    });
}

function load_bin_file(file, files, on_all_loaded) {
    load_arraybuffer(file, function(ab) {
        console.log("loaded " + file);
        files[file] = ab;
        if (all_loaded(files)) {
            on_all_loaded(files);
        }
    });
}

function export_game(name, code) {
    log_output("Generating export...", "info");
    var files = {};
    function create_export(files) {
        var data_zip = new JSZip();
        data_zip.file("main.lua", code);
        var data_base64 = data_zip.generate({});

        var download_zip = new JSZip();
        download_zip.file("data.pak", data_base64, {base64: true});
        download_zip.file("amulet.js", files["amulet.js"]);
        download_zip.file("amulet.wasm", files["amulet.wasm"]);
        download_zip.file("amulet_license.txt", files["amulet_license.txt"]);
        download_zip.file("index.html", files["player.html"]);
        var content = download_zip.generate({type:"blob"});
        saveAs(content, name + ".zip");

        log_output("Export complete!", "success");
        log_output("A file called " + name + ".zip should have just downloaded.");
        log_output("You can now upload this file to a hosting site such as <http://itch.io> or <http://gamejolt.com>.");
        log_output("Note that the game needs to be hosted on a webserver to work.");
        log_output("For a native standalone build, see the documentation at <http://www.amulet.xyz>.");
    }
    load_text_file("amulet.js", files, create_export);
    load_bin_file("amulet.wasm", files, create_export);
    load_text_file("amulet_license.txt", files, create_export);
    load_text_file("player.html", files, create_export);
}
