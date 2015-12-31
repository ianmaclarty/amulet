#!/usr/bin/env node
var fs = require("fs");
var http = require("https");
var jszip = require("./jszip.js");
var parse_url = require("url").parse;
var log = console.log;

var tag = process.argv[2];
if (!tag) throw "no tag";
var token = process.env.GITHUB_TOKEN;
if (!token) throw "no GITHUB_TOKEN";

var host = "https://api.github.com";
var owner = "ianmaclarty";
var repo = "amulet";
var access = "access_token=" + token;

function get(url, cb) {
    var options = parse_url(url);
    options.headers = {"User-Agent": owner};
    http.request(options, function(response) {
        var str = "";
        response.on("data", function(chunk) {
            str += chunk;
        });
        response.on("end", function () {
            cb(JSON.parse(str), response.statusCode);
        });
    }).end();
}

function post(url, data, type, cb) {
    var options = parse_url(url);
    options.headers = {
        "User-Agent": owner,
        "Content-Length": data.length,
        "Content-Type": type,
    };
    options.method = "POST";
    var req = http.request(options, function(response) {
        var str = "";
        response.on("data", function(chunk) {
            str += chunk;
        });
        response.on("end", function () {
            cb(JSON.parse(str), response.statusCode);
        });
    })
    req.write(data);
    req.end();
}

function query_tag_release(cb) {
    get(host+"/repos/"+owner+"/"+repo+"/releases/tags/"+tag+"?"+access, cb);
}

function get_tag_ref(tag, cb) {
    get(host+"/repos/"+owner+"/"+repo+"/git/refs/tags/"+tag+"?"+access, cb);
}

function create_tag_ref(tag, sha, cb) {
    var req = {
        ref: "refs/tags/"+tag,
        sha: sha,
    };
    post(host+"/repos/"+owner+"/"+repo+"/git/refs?"+access, JSON.stringify(req), "application/json", cb);
}

function create_release(cb) {
    var create_req = JSON.stringify({
        tag_name: tag,
        name: tag,
        draft: false,
        prerelease: true,
    });
    post(host+"/repos/"+owner+"/"+repo+"/releases?"+access, create_req,
        "application/json", function(resp, code)
    {
        cb(resp);
    });
}

function get_release(cb) {
    query_tag_release(function(resp, code) {
        if (code == 404) {
            // release does not exist yet, create it
            create_release(cb);
        } else {
            // release already exists
            cb(resp);
        }
    });
}

function upload_asset(upload_url, data, name, type, cb) {
    post(upload_url+"?name=" + name + "&" + access, data, type, function(resp, code) {
        if (Math.floor(code / 100) == 2) {
            cb();
        } else {
            throw "unable to upload file " + file + ": " + JSON.stringify(resp);
        }
    });
}

function upload_builds(upload_url, cb) {
    var zip = new jszip();
    var platforms = fs.readdirSync("builds");
    for (var p in platforms) {
        var platform = platforms[p];
        var luavms = fs.readdirSync("builds/"+platform);
        for (var l in luavms) {
            var luavm = luavms[l];
            var grades = fs.readdirSync("builds/"+platform+"/"+luavm);
            for (var g in grades) {
                var grade = grades[g];
                var dir = "builds/"+platform+"/"+luavm+"/"+grade+"/bin";
                var files = fs.readdirSync(dir);
                for (var f in files) {
                    var file = files[f];
                    var path = dir + "/" + file;
                    zip.file(path, fs.readFileSync(path));
                }
            }
        }
    }
    var data = zip.generate({
        compression: "DEFLATE",
        type: "nodebuffer",
    });
    var zipname = "builds-" + process.platform + ".zip";
    log("uploading " + zipname + "...");
    upload_asset(upload_url, data, zipname, "application/zip", cb);
}

log("uploading builds to github...");

get_release(function(release) {
    var upload_url = release.upload_url.replace(/\{.*$/, "")
    upload_builds(upload_url, function() {
        get_release(function(release) {
            var assets = release.assets;
            var count = 0;
            for (var a in assets) {
                var asset = assets[a];
                if (asset.name == "builds-darwin.zip" ||
                    asset.name == "builds-win32.zip" ||
                    asset.name == "builds-linux.zip")
                {
                    count++;
                }
            }
            if (count == 3) {
                log("all uploads done, triggering distro builds...");
                get_tag_ref(tag, function(ref) {
                    create_tag_ref(tag+"-distro-trigger", ref.object.sha, function() {
                        log("done");
                    });
                });
            } else {
                log("done");
            }
        });
    });
});
