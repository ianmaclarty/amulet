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

var distros = [];
for (var d = 3; d < process.argv.length; d++) {
    distros.push(process.argv[d]);
}
if (distros.length == 0) throw "no distros";

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

function get_release(cb) {
    query_tag_release(function(resp, code) {
        if (code == 404) {
            throw "release " + tag + " does not exist";
        } else {
            cb(resp);
        }
    });
}

function upload_asset(upload_url, data, name, type, cb) {
    post(upload_url+"?name=" + name + "&" + access, data, type, function(resp, code) {
        if (Math.floor(code / 100) == 2) {
            cb();
        } else {
            throw "unable to upload file " + name + ": " + code + ": " + JSON.stringify(resp, null, 2);
        }
    });
}

log("uploading distros to github...");

get_release(function(release) {
    var upload_url = release.upload_url.replace(/\{.*$/, "")
    function upload_distro(d, cb) {
        if (d < distros.length) {
            var file = distros[d];
            log("uploading " + file + "...");
            upload_asset(upload_url, fs.readFileSync(file), file, "application/octet-stream", function() {
                upload_distro(d+1, cb);
            });
        } else {
            cb();
        }
    }
    upload_distro(0, function() {
        log("done");
    });
});
