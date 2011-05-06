#!/bin/sh
# Default locale is russian
SYS_LOCALE=ru_RU.UTF-8

# This can be overrided by setting kernel cmdline option LOCALE, for example, LOCALE=en_US.UTF-8
# Please note that you should provide full locale name, including UTF-8 or whatever you want.
OVERRIDE_LOCALE=`cat /proc/cmdline | sed 's/^.*LOCALE=//g' | sed 's/\ .*//g'`

if [ "$OVERRIDE_LOCALE" != "" ] ; then
	SYS_LOCALE=$OVERRIDE_LOCALE
fi

export LANG=${SYS_LOCALE}
export LC_CTYPE=${SYS_LOCALE}
export LC_NUMERIC=${SYS_LOCALE}
export LC_TIME=${SYS_LOCALE}
export LC_COLLATE=C
export LC_MONETARY=${SYS_LOCALE}
export LC_MESSAGES=${SYS_LOCALE}
export LC_PAPER=${SYS_LOCALE}
export LC_NAME=${SYS_LOCALE}
export LC_ADDRESS=${SYS_LOCALE}
export LC_TELEPHONE=${SYS_LOCALE}
export LC_MEASUREMENT=${SYS_LOCALE}
export LC_IDENTIFICATION=${SYS_LOCALE}
export LESSCHARSET=UTF-8


