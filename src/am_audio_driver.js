//"use strict";

var LibraryAMA = {
    $AMA__deps: ['$Browser'],
    $AMA: {
        context: null,
        idmap: {},
        nextid: 1,
        destid: null,
        make_id: function (obj) {
            var id = AMA.nextid;
            AMA.nextid++;
            AMA.idmap[id] = obj;
            return id;
        },
        del_id: function (id) {
            var map = AMA.idmap;
            delete map[id];
        }
    },
    AMA_init: function () {
        var ctx;
        try {
            ctx = new AudioContext();
        } catch (e) {
            try {
                ctx = new webkitAudioContext();
            } catch (e) {}
        }

        if (ctx) {
            // Old Web Audio API (e.g. Safari 6.0.5) had an inconsistently named createGainNode function.
            if (typeof(ctx.createGain) === 'undefined') ctx.createGain = ctx.createGainNode;
            AMA.context = ctx;
            AMA.destid = AMA.make_id(ctx.destination);
        } else {
            console.log('Web Audio not supported.');
        }
    },
    AMA_connect: function(from_id, to_id) {
        if (!AMA.context) return;
        var from_node = AMA.idmap[from_id];
        var to_node = AMA.idmap[to_id];
        from_node.connect(to_node);
    },
    AMA_speakers: function() {
        if (!AMA.context) return 0;
        return AMA.destid;
    },
    AMA_oscillator_create: function () {
        if (!AMA.context) return 0;
        return AMA.make_id(AMA.context.createOscillator());
    },
    AMA_oscillator_delete: function (id) {
        if (!AMA.context) return;
        AMA.del_id(id);
    },
    AMA_oscillator_start: function(id, when) {
        if (!AMA.context) return;
        AMA.idmap[id].start(when);
    },
    AMA_oscillator_stop: function(id, when) {
        if (!AMA.context) return;
        AMA.idmap[id].stop(when);
    }
};

autoAddDeps(LibraryAMA, '$AMA');
mergeInto(LibraryManager.library, LibraryAMA);
