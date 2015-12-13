#!/usr/bin/env node
var fs = require("fs");
var http = require("https");
var jszip = require("./jszip.js");
var parse_url = require("url").parse;
var log = console.log;

var tag = process.argv[2]
if (!tag) throw "no tag"
var token = process.env.GITHUB_TOKEN
if (!token) throw "no GITHUB_TOKEN"

var host = "https://api.github.com"
var owner = "ianmaclarty"
var repo = "amulet"
var access = "access_token=" + token

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

function query_tag(cb) {
    get(host+"/repos/"+owner+"/"+repo+"/releases/tags/"+tag+"?"+access, cb);
}

function create_release(cb) {
    var create_req = JSON.stringify({
        tag_name: tag,
        name: tag,
    });
    post(host+"/repos/"+owner+"/"+repo+"/releases?"+access, create_req,
        "application/json", function(resp, code)
    {
        cb(resp.id, resp.upload_url.replace(/\{.*$/, ""));
    });
}

function get_release(cb) {
    query_tag(function(resp, code) {
        if (code == 404) {
            // release does not exist yet, create it
            create_release(cb);
        } else {
            // release already exists
            cb(resp.id, resp.upload_url.replace(/\{.*$/, ""));
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

function get_builds() {
    var builds = [];
    var platforms = fs.readdirSync("builds");
    for (var p in platforms) {
        var platform = platforms[p];
        var luavms = fs.readdirSync("builds/"+platform);
        for (var l in luavms) {
            var luavm = luavms[l];
            var grades = fs.readdirSync("builds/"+platform+"/"+luavm);
            for (var g in grades) {
                var grade = grades[g];
                builds.push({
                    path: "builds/"+platform+"/"+luavm+"/"+grade+"/bin",
                    platform: platform,
                    luavm: luavm,
                    grade: grade,
                });
            }
        }
    }
    return builds;
}

function upload_build(upload_url, build, cb) {
    var zip = new jszip();
    var files = fs.readdirSync(build.path);
    for (var f in files) {
        var file = files[f];
        zip.file(file, fs.readFileSync(build.path+"/"+file));
    }
    var data = zip.generate({
        compression: "DEFLATE",
        type: "nodebuffer",
    });
    var name = "amulet-"+tag+"-"+build.platform+"-"+build.luavm+"-"+build.grade+".zip";
    log("uploading " + name + "...");
    upload_asset(upload_url, data, name, "application/zip", cb);
}

log("uploading builds to github...");

get_release(function(id, upload_url) {
    var builds = get_builds();
    function upload(b, cb) {
        if (b < builds.length) {
            upload_build(upload_url, builds[b], function() { upload(b+1, cb); });
        } else {
            cb();
        }
    }
    upload(0, function() {
        log("done");
    });
})
