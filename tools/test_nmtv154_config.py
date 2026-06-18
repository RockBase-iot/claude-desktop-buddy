from pathlib import Path


ini = Path("platformio.ini").read_text(encoding="utf-8")
start = ini.index("[env:nm-tv-154]")
end = ini.find("\n[env:", start + 1)
section = ini[start:] if end == -1 else ini[start:end]

assert "-DTFT_BL=-1" not in section, "NMTV154 must not define TFT_BL as -1"
assert "-DTFT_RST=-1" in section, "NMTV154 keeps TFT_RST disabled explicitly"
