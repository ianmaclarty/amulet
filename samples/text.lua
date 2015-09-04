local am = amulet

local win = am.window{width = 800, height = 600}

local strs = {
[[
Stay, you imperfect speakers, tell me more:
By Sinel's death I know I am thane of Glamis;
But how of Cawdor? the thane of Cawdor lives,
A prosperous gentleman; and to be king
Stands not within the prospect of belief,
No more than to be Cawdor. Say from whence
You owe this strange intelligence? or why
Upon this blasted heath you stop our way
With such prophetic greeting? Speak, I charge you.]],
[[
As whence the sun 'gins his reflection
Shipwrecking storms and direful thunders break,
So from that spring whence comfort seem'd to come
Discomfort swells. Mark, king of Scotland, mark:
No sooner justice had with valour arm'd
Compell'd these skipping kerns to trust their heels,
But the Norweyan lord surveying vantage,
With furbish'd arms and new supplies of men
Began a fresh assault.]],
[[
Yes;
As sparrows eagles, or the hare the lion.
If I say sooth, I must report they were
As cannons overcharged with double cracks, so they
Doubly redoubled strokes upon the foe:
Except they meant to bathe in reeking wounds,
Or memorise another Golgotha,
I cannot tell.
But I am faint, my gashes cry for help.]],
[[
From Fife, great king;
Where the Norweyan banners flout the sky
And fan our people cold. Norway himself,
With terrible numbers,
Assisted by that most disloyal traitor
The thane of Cawdor, began a dismal conflict;
Till that Bellona's bridegroom, lapp'd in proof,
Confronted him with self-comparisons,
Point against point rebellious, arm 'gainst arm.
Curbing his lavish spirit: and, to conclude,
The victory fell on us.]],
[[
I myself have all the other,
And the very ports they blow,
All the quarters that they know
I' the shipman's card.
I will drain him dry as hay:
Sleep shall neither night nor day
Hang upon his pent-house lid;
He shall live a man forbid:
Weary se'nnights nine times nine
Shall he dwindle, peak and pine:
Though his bark cannot be lost,
Yet it shall be tempest-tost.
Look what I have.]]
}

win.root = am.group(
        am.text(strs[1], "left", "center")
            :action(coroutine.create(function(node)
                while true do
                    local str
                    repeat
                        str = strs[math.random(#strs)]
                    until str ~= node.text
                    node.text = str
                    am.wait(am.delay(2))
                end
            end))
    )
    :scale("MV", vec3(1))
    :translate("MV", vec3(200, 300, 0))
    :bind{
        MV = mat4(1),
        P = math.ortho(0, win.width, 0, win.height)
    }
    :action(function()
        if win:key_pressed("escape") then
            win:close()
        end
    end)
