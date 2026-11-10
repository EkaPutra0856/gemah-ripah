"""Renders the full-song trailer picture from plan2.EDIT, then muxes it with trailer2_audio.wav."""
import os
import subprocess

import plan2 as plan

OUT = "E:/NewSSDFile/Game Pemerintahan/GemahRipah/Saved/Rekaman/Trailer_GemahRipah_Full.mp4"
FONT = "C\\:/Windows/Fonts/segoeuib.ttf"
FONT_REG = "C\\:/Windows/Fonts/segoeui.ttf"
ORANGE = "0xE8A33A"
W, H, FPS = 1920, 1080, 30
GRADE = "scale=%d:%d:flags=lanczos,eq=contrast=1.06:saturation=1.12,unsharp=5:5:0.4" % (W, H)
os.chdir(plan.HERE)
os.makedirs("parts", exist_ok=True)
# The recordings caught the Windows activation notice in the bottom right corner, and the cutscenes show "Esc: lewati".
# Gameplay drops a 16:9 strip off the right and bottom edges, the cutscenes paint over their black bars.
WATERMARK = "crop=1560:878:20:0,"
CLEAN = {"cs": "drawbox=x=1440:y=0:w=160:h=42:color=black:t=fill,drawbox=x=1440:y=860:w=160:h=40:color=black:t=fill,",
         "gp": WATERMARK, "oj": WATERMARK}


def text_file(name, text):
    path = "parts/%s.txt" % name
    open(path, "w", encoding="utf8").write(text)
    return path.replace(":", "\\:")


def alpha(t0, t1, fin, fout):
    """drawtext alpha expression: fade in at t0, out at t1."""
    return "if(lt(t,%f),0,if(lt(t,%f),(t-%f)/%f,if(lt(t,%f),1,if(lt(t,%f),(%f-t)/%f,0))))" % (
        t0, t0 + fin, t0, fin, t1 - fout, t1, t1, fout)


def run(args):
    r = subprocess.run(["ffmpeg", "-v", "error", "-y"] + args, capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(r.stderr)


ENC = ["-r", str(FPS), "-c:v", "libx264", "-crf", "12", "-preset", "fast", "-pix_fmt", "yuv420p", "-an"]
names = []
TL = plan.timeline()
for i, (kind, what, start, _beats, opt) in enumerate(plan.EDIT):
    out = "parts/%02d.mp4" % i
    names.append(out)
    # Rounded on the running position, so the cuts never drift from the beats in the soundtrack.
    t0, dur = TL[i]
    frames = round((t0 + dur) * FPS) - round(t0 * FPS)
    if kind == "clip":
        vf = CLEAN.get(what, "") + GRADE
        if opt.get("fadein"):
            vf += ",fade=t=in:st=0:d=%f" % opt["fadein"]
        run(["-ss", str(start), "-i", plan.SRC[what], "-frames:v", str(frames), "-vf", vf] + ENC + [out])
    elif kind == "card":
        vf = "null"
        if what:
            tf = text_file("card%02d" % i, what)
            a = alpha(0.08, dur - 0.05, 0.18, 0.15)
            vf = ("drawtext=fontfile='%s':textfile='%s':fontsize=66:fontcolor=white:x=(w-tw)/2:y=(h-th)/2-10:alpha='%s',"
                  "drawbox=x=(iw-160)/2:y=ih/2+52:w=160:h=4:color=%s@0.9:t=fill:enable='between(t,0.12,%f)'") % (
                FONT, tf, a, ORANGE, dur - 0.08)
        run(["-f", "lavfi", "-i", "color=c=0x0b0b0d:s=%dx%d:r=%d" % (W, H, FPS), "-frames:v", str(frames), "-vf", vf] + ENC + [out])
    elif kind == "title":
        # The Dewan from the opening cutscene, blurred and darkened behind the title.
        t1 = text_file("title", "GEMAH RIPAH")
        t2 = text_file("sub", "Suara rakyat bukan barang dagangan.")
        t3 = text_file("tag", "Satire open world tentang rakyat biasa  |  Segera di PC")
        vf = (CLEAN["cs"] + "setpts=1.8*PTS,fps=30,tpad=stop_mode=clone:stop_duration=5," + GRADE + ",boxblur=12:2,eq=brightness=-0.22:saturation=0.7,"
              "zoompan=z='1.0+0.0006*on':x='iw/2-(iw/zoom/2)':y='ih/2-(ih/zoom/2)':d=1:s=%dx%d:fps=%d,"
              "drawtext=fontfile='%s':textfile='%s':fontsize=170:fontcolor=%s:x=(w-tw)/2:y=(h-th)/2-90:alpha='%s',"
              "drawtext=fontfile='%s':textfile='%s':fontsize=52:fontcolor=white:x=(w-tw)/2:y=h/2+40:alpha='%s',"
              "drawtext=fontfile='%s':textfile='%s':fontsize=34:fontcolor=0xd8d8d8:x=(w-tw)/2:y=h/2+130:alpha='%s',"
              "fade=t=in:st=0:d=0.25,fade=t=out:st=%f:d=2.6") % (
            W, H, FPS, FONT, t1, ORANGE, alpha(0.15, dur, 0.5, 0.1),
            FONT_REG, t2, alpha(1.4, dur, 0.5, 0.1), FONT_REG, t3, alpha(2.5, dur, 0.6, 0.1), dur - 2.6)
        run(["-ss", "33.0", "-i", plan.SRC["cs"], "-frames:v", str(frames), "-vf", vf] + ENC + [out])
    print(out, kind, dur)

with open("parts/list.txt", "w") as f:
    f.writelines("file '%s'\n" % os.path.basename(n) for n in names)
run(["-f", "concat", "-safe", "0", "-i", "parts/list.txt", "-c", "copy", "parts/picture.mp4"])
run(["-i", "parts/picture.mp4", "-i", os.path.join(plan.HERE, "trailer2_audio.wav"), "-map", "0:v", "-map", "1:a", "-c:v", "libx264", "-crf", "20",
     "-preset", "slow", "-profile:v", "high", "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "192k", "-t", "%.3f" % plan.SONG_LEN,
     "-movflags", "+faststart", OUT])
print("done", OUT, os.path.getsize(OUT) // 1024 // 1024, "MB")
