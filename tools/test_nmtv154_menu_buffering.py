from pathlib import Path


src = Path("src/main.cpp").read_text(encoding="utf-8")
menu_start = src.index("#ifdef NMTV154_BOARD", src.index("void drawMenu()"))
menu_end = src.index("#else", menu_start)
menu_branch = src[menu_start:menu_end]

assert "TFT_eSprite" not in menu_branch, "NMTV154 menu should not use a second sprite"
assert "HAL_DISPLAY.fillRect" in menu_branch, "NMTV154 menu draws directly when dirty"
assert "nmtv154MenuDirty" in src, "NMTV154 menu must be guarded by a dirty flag"
assert "nmtv154MenuLastSel" in src, "NMTV154 menu must cache menu selection"
assert "nmtv154MenuLastDemo" in src, "NMTV154 menu must cache demo state"

loop_start = src.index("#if defined(NMTV154_BOARD)", src.index("if (landscapeClock)"))
loop_end = src.index("#else", loop_start)
loop_branch = src[loop_start:loop_end]

assert "drawMenu()" in loop_branch, "NMTV154 loop must still draw menu"
assert "nmtv154MenuDirty" in loop_branch, "NMTV154 loop must update menu dirty state"
