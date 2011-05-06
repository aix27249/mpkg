# Keyboard settings
Section "InputClass"
	Identifier "KeymapSettings"
	MatchIsKeyboard "on"
	Option      "AutoRepeat"  "250 30"
	Option      "XkbRules"    "xorg"
	Option      "XkbModel"    "pc105"
	Option      "XkbLayout"   "us,ru"
	Option      "XkbOptions"  "grp:alt_shift_toggle,grp_led:scroll"
EndSection
