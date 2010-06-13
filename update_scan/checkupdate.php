<?php
include "strverscmp.inc";

function getLftpListing($url) {
	exec("echo cls -B | lftp $url ", $output);
	return $output;
}

function getAllVersions($url, $search, $output) {
	//print "search: $search, url: $url\n";
	if (!isset($url)) die("URL not set");
	if (!isset($search)) die("Search not set");
	if (!isset($output)) die("Output not set");
	foreach($output as $link) {
		if (strpos("$link", "$search-")===false) continue;
		if (strpos($link, "tar.bz2")===false) continue;
		$pattern="/$search-/";
		$link=preg_replace($pattern, "", $link);
		$link=preg_replace("/\.tar.*$/", "", $link);
		$ret[]=$link;
	}
	return $ret;
}

function getLatestVersion($allVersions) {
	$latest = $allVersions[0];
	foreach($allVersions as $ver) {
		if (strverscmp($latest, $ver)<0) $latest = $ver;
	}
	return $latest;
}
/*$class="driver";
$url = "http://xorg.freedesktop.org/archive/individual/$class";
$search = "xf86-video-intel";
$output = getLftpListing($url);
$allVersions = getAllVersions($url, $search, $output);

$latestVersion = getLatestVersion($allVersions);

print "Latest version of $search is $latestVersion\n";
die("");*/
function getPkgName($link) {
	$search = substr($link, 0, strrpos($link, "-"));
	return $search;

}
function getPkgVersion($link) {
	$search = getPkgName($link);
	$pattern="/$search-/";
	$link=preg_replace($pattern, "", $link);
	$link=preg_replace("/\.tar.*$/", "", $link);
	return $link;

}

function getLV($fullurl) {
	$pos = strrpos($fullurl, "/");
	$url = substr($fullurl, 0, $pos);
	$search = getPkgName(substr($fullurl, $pos+1));
	print "[$url] [$search]\n";
	
}

//getLV("http://xorg.freedesktop.org/archive/individual/driver/xf86-video-nv-2.0.1.tar.bz2");

?>
