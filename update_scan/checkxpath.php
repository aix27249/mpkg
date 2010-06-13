<?php
include "checkupdate.php";
$classlist=array("app", "data", "doc", "driver", "font", "lib", "proto", "util", "xserver");
foreach($classlist as $class) {
	$url = "http://xorg.freedesktop.org/archive/individual/$class";
	$packagez = scandir("/tmp/xorg/src/$class");
	$output = getLftpListing($url);
	foreach($packagez as $fn) {
		if ($fn==="." || $fn==="..") continue;
		$search = getPkgName($fn);
		$all = getAllVersions($url, $search, $output);
		$latest = getLatestVersion($all);
		$current = getPkgVersion($fn);
		if (strverscmp($latest, $current)<0) $latest = $current;
		print "$search: $latest\n";
		if (file_exists("/tmp/xorg/src/$class/$search-$latest.tar.bz2")) {
			system("cp /tmp/xorg/src/$class/$search-$latest.tar.bz2 /tmp/xorg/newsrc/$class/$search-$latest.tar.bz2");
		}
		else if (file_exists("/tmp/xorg/src/$class/$search-$latest.tar.xz")) {
			system("cp /tmp/xorg/src/$class/$search-$latest.tar.xz /tmp/xorg/newsrc/$class/$search-$latest.tar.xz");
		}
		else if (file_exists("/tmp/xorg/src/$class/$search-$latest.tar.gz")) {
			system("cp /tmp/xorg/src/$class/$search-$latest.tar.gz /tmp/xorg/newsrc/$class/$search-$latest.tar.gz");
		}

		else {
			if (!file_exists("/tmp/xorg/newsrc/$class/$search-$latest.tar.bz2") && 
				!file_exists("/tmp/xorg/newsrc/$class/$search-$latest.tar.xz")  && 
				!file_exists("/tmp/xorg/newsrc/$class/$search-$latest.tar.gz")  ) {
					system("( cd /tmp/xorg/newsrc/$class ; wget $url/$search-$latest.tar.bz2 )");
			}
		}
	}
}
