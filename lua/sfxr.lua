local wave_type_code = {
    square = 0,
    sawtooth = 1,
    sine = 2,
    noise = 3,
}

local wave_types = {
 "sine",
 "square",
 "noise",
 "sawtooth",
}

local
function base_settings()
    return {
	wave_type = "square",
	base_freq = 0.3,
	freq_limit = 0.0,
	freq_ramp = 0.0,
	freq_dramp = 0.0,
	duty = 0.0,
	duty_ramp = 0.0,
	vib_strength = 0.0,
	vib_speed = 0.0,
	vib_delay = 0.0,
	env_attack = 0.0,
	env_sustain = 0.3,
	env_decay = 0.4,
	env_punch = 0.0,
	filter_on = false,
	lpf_resonance = 0.0,
	lpf_freq = 1.0,
	lpf_ramp = 0.0,
	hpf_freq = 0.0,
	hpf_ramp = 0.0,
	pha_offset = 0.0,
	pha_ramp = 0.0,
	repeat_speed = 0.0,
	arp_speed = 0.0,
	arp_mod = 0.0,
    }
end

local
function pickup(seed)
    local r = am.rand(seed)
    local settings = {}
    settings.base_freq = 0.4 + r() * 0.5
    settings.env_attack = 0.0
    settings.env_sustain = r() * 0.1
    settings.env_decay = 0.1 + r() * 0.4
    settings.env_punch = 0.3 + r() * 0.3
    if r(2) == 1 then
        settings.arp_speed = 0.5 + r() * 0.2;
        settings.arp_mod = 0.2 + r() * 0.4;
    end
    return settings
end

local
function shoot(seed)
    local r = am.rand(seed)
    local settings = {}
    local w = r(3)
    if w == 1 then
        settings.wave_type = "square"
    elseif w == 2 then
        settings.wave_type = "sawtooth"
    else
        if r(2) == 1 then
            settings.wave_type = "sine"
        else
            if r(2) == 1 then
                settings.wave_type = "square"
            else
                settings.wave_type = "sawtooth"
            end
        end
    end
    settings.base_freq = 0.5 + r() * 0.5
    settings.freq_limit = math.max(0.2, settings.base_freq - 0.2 - r() * 0.6)
    settings.freq_ramp = -0.15 - r() * 0.2
    if r(3) == 1 then
        settings.base_freq = 0.3 + r() * 0.6
        settings.freq_limit = r() * 0.1
        settings.freq_ramp = -0.35 - r() * 0.3
    end
    if r(2) == 1 then
        settings.duty = r() * 0.5
        settings.duty_ramp = r() * 0.2
    else
        settings.duty = 0.4 + r() * 0.5
        settings.duty_ramp = -r() * 0.7
    end
    settings.env_attack = 0.0
    settings.env_sustain = 0.1 + r() * 0.2
    settings.env_decay = r() * 0.4
    if r(2) == 1 then
        settings.env_punch = r() * 0.3
    end
    if r(3) == 1 then
        settings.pha_offset = r() * 0.2
        settings.pha_ramp = -r() * 0.2
    end
    if r(2) == 1 then
        settings.hpf_freq = r() * 0.3
    end
    return settings
end

local
function explosion(seed)
    local r = am.rand(seed)
    local settings = {}
    settings.wave_type = "noise";
    if r(2) == 1 then
        settings.base_freq=0.1+r() * 0.4
        settings.freq_ramp=-0.1+r() * 0.4
    else
        settings.base_freq=0.2+r() * 0.7
        settings.freq_ramp=-0.2-r() * 0.2
    end
    settings.base_freq = settings.base_freq * settings.base_freq
    if r(5) == 1 then
        settings.freq_ramp=0.0
    end
    if r(3) == 1 then
        settings.repeat_speed=0.3+r() * 0.5
    end
    settings.env_attack=0.0
    settings.env_sustain=0.1+r() * 0.3
    settings.env_decay=r() * 0.5
    if r(2) == 1 then
        settings.pha_offset=-0.3+r() * 0.9
        settings.pha_ramp=-r() * 0.3
    end
    settings.env_punch=0.2+r() * 0.6
    if r(2) == 1 then
        settings.vib_strength=r() * 0.7
        settings.vib_speed=r() * 0.6
    end
    if r(3) == 1 then
        settings.arp_speed=0.6+r() * 0.3
        settings.arp_mod=0.8-r() * 1.6
    end
    return settings
end

local
function powerup(seed)
    local r = am.rand(seed)
    local settings = {}
    if r(2) == 1 then
        settings.wave_type = "sawtooth"
    else
        settings.duty=r() * 0.6
    end
    if r(2) == 1 then
        settings.base_freq=0.2+r() * 0.3
        settings.freq_ramp=0.1+r() * 0.4
        settings.repeat_speed=0.4+r() * 0.4
    else
        settings.base_freq=0.2+r() * 0.3
        settings.freq_ramp=0.05+r() * 0.2
        if r(2) == 1 then
            settings.vib_strength=r() * 0.7
            settings.vib_speed=r() * 0.6
        end
    end
    settings.env_attack=0.0
    settings.env_sustain=r() * 0.4
    settings.env_decay=0.1+r() * 0.4
    return settings
end

local
function hit(seed)
    local r = am.rand(seed)
    local settings = {}
    local w = r(3)
    if w == 1 then
        settings.wave_type = "square"
    elseif w == 2 then
        settings.wave_type = "sawtooth"
    else
        settings.wave_type = "noise"
    end
    if w == 1 then
        settings.duty=r() * 0.6
    end
    settings.base_freq=0.2+r() * 0.6
    settings.freq_ramp=-0.3-r() * 0.4
    settings.env_attack=0.0
    settings.env_sustain=r() * 0.1
    settings.env_decay=0.1+r() * 0.2
    if r(2) == 1 then
        settings.hpf_freq=r() * 0.3
    end
    return settings
end

local
function jump(seed)
    local r = am.rand(seed)
    local settings = {}
    settings.wave_type="square"
    settings.duty=r() * 0.6
    settings.base_freq=0.3+r() * 0.3
    settings.freq_ramp=0.1+r() * 0.2
    settings.env_attack=0.0
    settings.env_sustain=0.1+r() * 0.3
    settings.env_decay=0.1+r() * 0.2
    if r(2) == 1 then
        settings.hpf_freq=r() * 0.3
    end
    if r(2) == 1 then
        settings.lpf_freq=1.0-r() * 0.6
    end
    return settings
end

local
function blip(seed)
    local r = am.rand(seed)
    local settings = {}
    if r(2) == 1 then
        settings.wave_type = "square"
        settings.duty=r() * 0.6
    else
        settings.wave_type = "sawtooth"
    end
    settings.base_freq=0.2+r() * 0.4
    settings.env_attack=0.0
    settings.env_sustain=0.1+r() * 0.1
    settings.env_decay=r() * 0.2
    settings.hpf_freq=0.1
    return settings
end

local
function randomg(seed)
    local r = am.rand(seed)
    local settings = {}
    settings.wave_type = wave_types[r(#wave_types)]
    settings.base_freq = (r() * 2.0 - 1.0) ^ 2.0
    if r(2) == 1 then
        settings.base_freq = (r() * 2.0 - 1.0) ^ 3.0 + 0.5
    end
    settings.freq_limit = 0.0
    settings.freq_ramp = (r() * 2.0 - 1.0) ^ 5.0
    if settings.base_freq > 0.7 and settings.freq_ramp > 0.2 then
        settings.freq_ramp = -settings.freq_ramp
    end
    if settings.base_freq < 0.2 and settings.freq_ramp < -0.05 then
        settings.freq_ramp = -settings.freq_ramp
    end
    settings.freq_dramp = (r() * 2.0 - 1.0) ^ 3.0
    settings.duty = r() * 2.0 - 1.0
    settings.duty_ramp = (r() * 2.0 - 1.0) ^ 3.0
    settings.vib_strength = (r() * 2.0 - 1.0) ^ 3.0
    settings.vib_speed = r() * 2.0 - 1.0
    settings.env_attack = (r() * 2.0 - 1.0) ^ 3.0
    settings.env_sustain = (r() * 2.0 - 1.0) ^ 2.0
    settings.env_decay = r() * 2.0 - 1.0
    settings.env_punch = (r() * 0.8) ^ 2.0
    if settings.env_attack + settings.env_sustain + settings.env_decay < 0.2 then
        settings.env_sustain = settings.env_sustain + 0.2 + r() * 0.3
        settings.env_decay = settings.env_decay + 0.2 + r() * 0.3
    end
    settings.lpf_resonance = r() * 2.0 - 1.0
    settings.lpf_freq = 1.0 - (r() * 1.0) ^ 3.0
    settings.lpf_ramp = (r() * 2.0 - 1.0) ^ 3.0
    if settings.lpf_freq < 0.1 and settings.lpf_ramp < -0.05 then
        settings.lpf_ramp = -settings.lpf_ramp
    end
    settings.hpf_freq = r() ^ 5.0
    settings.hpf_ramp = (r() * 2.0 - 1.0) ^ 5.0
    settings.pha_offset = (r() * 2.0 - 1.0) ^ 3.0
    settings.pha_ramp = (r() * 2.0 - 1.0) ^ 3.0
    settings.repeat_speed = r() * 2.0 - 1.0
    settings.arp_speed = r() * 2.0 - 1.0
    settings.arp_mod = r() * 2.0 - 1.0
    return settings
end

local
function bird(seed)
    local r = am.rand(seed)
    local settings = {}

    settings.wave_type = wave_types[r(#wave_types)]
    if (settings.wave_type == "sawtooth" or settings.wave_type == "noise") then
      settings.wave_type = "sine"
    end
    settings.base_freq = 0.85 + (r() * 0.15)
    settings.freq_ramp = 0.3 + (r() * 0.15)

    settings.env_attack = 0 + (r() * 0.09)
    settings.env_sustain = 0.2 + (r() * 0.3)
    settings.env_decay = 0 + (r() * 0.1)

    settings.duty = (r() * 2.0) - 1.0
    settings.duty_ramp = ((r() * 2.0) - 1.0) ^ 3.0


    settings.repeat_speed = 0.5 + (r() * 0.1)

    settings.pha_offset = -0.3 + (r() * 0.9)
    settings.pha_ramp = -(r() * 0.3)

    settings.arp_speed = 0.4 + (r() * 0.6)
    settings.arp_mod = 0.8 + (r() * 0.1)

    settings.lpf_resonance = (r() * 2.0) - 1.0
    settings.lpf_freq = 1.0 - (r() * 1.0) ^ 3.0
    settings.lpf_ramp = (r() * 2.0 - 1.0) ^ 3.0
    if (settings.lpf_freq < 0.1 and settings.lpf_ramp < -0.05) then
      settings.lpf_ramp = -settings.lpf_ramp
    end
    settings.hpf_freq = r() ^ 5.0
    settings.hpf_ramp = (r() * 2.0 - 1.0) ^ 5.0
    return settings
end

local
function push(seed)
    local r = am.rand(seed)
    local settings = {}

    settings.wave_type = wave_types[r(#wave_types)]
    if (settings.wave_type == "sine" or settings.wave_type == "square") then
        settings.wave_type = "noise"
    end
    settings.base_freq = 0.1 + (r() * 0.4)
    settings.freq_ramp = 0.05 + (r() * 0.2)

    settings.env_attack = 0.01 + (r() * 0.09)
    settings.env_sustain = 0.01 + (r() * 0.09)
    settings.env_decay = 0.01 + (r() * 0.09)

    settings.repeat_speed = 0.3 + (r() * 0.5)
    settings.pha_offset = -0.3 + (r() * 0.9)
    settings.pha_ramp = -(r() * 0.3)
    settings.arp_speed = 0.6 + (r() * 0.3)
    settings.arp_mod = 0.8 - (r() * 1.6)
    return settings
end

local generators = {
    pickup,
    shoot,
    explosion,
    powerup,
    hit,
    jump,
    blip,
    bird,
    push,
    randomg,
}

function am.sfxr_synth(arg)
    local settings = base_settings()
    local t = type(arg)
    if t == "table" then
        table.merge(settings, arg)
    elseif t == "number" then
        local index = (math.floor(arg) % 100) % #generators + 1
        local gen = generators[index]
        table.merge(settings, gen(math.floor(arg / 100)))
    end
    local w = wave_type_code[settings.wave_type]
    if not w then
        error("invalid wave type: "..settings.wave_type, 2)
    end
    local buf = am._sfxr(
	wave_type_code[settings.wave_type],
	settings.base_freq,
	settings.freq_limit,
	settings.freq_ramp,
	settings.freq_dramp,
	settings.duty,
	settings.duty_ramp,
	settings.vib_strength,
	settings.vib_speed,
	settings.vib_delay,
	settings.env_attack,
	settings.env_sustain,
	settings.env_decay,
	settings.env_punch,
	settings.filter_on,
	settings.lpf_resonance,
	settings.lpf_freq,
	settings.lpf_ramp,
	settings.hpf_freq,
	settings.hpf_ramp,
	settings.pha_offset,
	settings.pha_ramp,
	settings.repeat_speed,
	settings.arp_speed,
	settings.arp_mod
    )
    return buf
end
