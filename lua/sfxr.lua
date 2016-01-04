local wave_type_code = {
    square = 0,
    sawtooth = 1,
    sine = 2,
    noise = 3,
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

local generators = {
    pickup = pickup,
    shoot = shoot,
    explosion = explosion,
    powerup = powerup,
    hit = hit,
    jump = jump,
    blip = blip,
}

local rand

function am.sfxr(arg1, arg2)
    local settings = base_settings()
    local seed
    local t = type(arg1)
    if t == "table" then
        table.merge(settings, arg1)
    elseif t == "string" then
        local gen = generators[arg1]
        if not gen then
            error("invalid generator: "..arg1)
        end
        if type(arg2) == "number" then
            seed = arg2
        else
            if not rand then
                rand = am.rand(os.time())
            end
            seed = rand(1000000)
        end
        table.merge(settings, gen(seed))
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
    return buf, seed
end
