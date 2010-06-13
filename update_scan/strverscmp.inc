<?php
function splitChunks($version) {
	// Replacing _ with .
	$v1=preg_replace("/_/", '.', $version);
	$length = strlen($v1);
	$d[]=0;
	for ($i=1; $i<$length; $i++) {
		if ($v1[$i-1]=='.') $d[]=$i;
		else {
			if ($v1[$i]<'a') {
				if ($v1[$i-1]>'9') $d[]=$i;
			}
			else {
				if ($v1[$i-1]<'a') $d[]=$i;
			}
		}
	}
	$f="";
	if (sizeof($d)==1) return $f;
	return $d;

}
function strverscmp($version1, $version2) {
	$d1=splitChunks($version1);
	$d2=splitChunks($version2);
	global $strver_debug;
	$chunks1 = sizeof($d1);
	$chunks2 = sizeof($d2);
	//if ($strver_debug==1) {
		//print "Comparing $version1 vs $version2<br>";
		//print "first: $chunks1, second: $chunks2<br>";
		/*print "CHUNKS1:<br>";
		for($i=1; $i<$chunks1+1; $i++) {
			if ($i<$chunks1) $s1 = preg_replace('/[.]/', '', substr($version1, $d1[$i-1], $d1[$i]-$d1[$i-1]));
			else if ($i==$chunks1) $s1 =  preg_replace('/[.]/', '', substr($version1, $d1[$i-1]));
			else $s1=0;
			if ($s1==="rc") $s1=-2;
			else if ($s1==="pre") $s1=-3;
			else if ($s1==="beta") $s1=-4;
			else if ($s1==="alpha") $s1=-5;
			else if ($s1==="prealpha") $s1=-6;
			else if ($s1==="git" || $s1==="svn" || $s1==="hg" || $s1==="r" || $s1==="rev" || $s1==="cvs") $s1=-7;

			print "$s1<br>";
		}
		print "CHUNKS2:<br>";
		for($i=1; $i<$chunks2+1; $i++) {
			if ($i<$chunks2) $s2 = preg_replace('/[.]/', '', substr($version2, $d2[$i-1], $d2[$i]-$d2[$i-1]));
			else if ($i==$chunks2) $s2 =  preg_replace('/[.]/', '', substr($version2, $d2[$i-1]));
			else $s2=0;
			if ($s2==="rc") $s2=-2;
			else if ($s2==="pre") $s2=-3;
			else if ($s2==="beta") $s2=-4;
			else if ($s2==="alpha") $s2=-5;
			else if ($s2==="prealpha") $s2=-6;
			else if ($s2==="git" || $s2==="svn" || $s2==="hg" || $s2==="r" || $s2==="rev" || $s1==="cvs") $s2=-7;

			print "$s2<br>";
		}*/
	//}
	$version1=preg_replace("/_/", '.', $version1);
	$version2=preg_replace("/_/", '.', $version2);

	for ($i=1; $i<$chunks1+1 || $i<$chunks2+1; $i++) {
		//print "Iteration $i<br>";

		if ($i<$chunks1) $s1 = preg_replace('/[.]/', '', substr($version1, $d1[$i-1], $d1[$i]-$d1[$i-1]));
		else if ($i==$chunks1) $s1 =  preg_replace('/[.]/', '', substr($version1, $d1[$i-1]));
		else $s1="0";

		if ($i<$chunks2) $s2 = preg_replace('/[.]/', '', substr($version2, $d2[$i-1], $d2[$i]-$d2[$i-1]));
		else if ($i==$chunks2) $s2 =  preg_replace('/[.]/', '', substr($version2, $d2[$i-1]));
		else $s2="0";

		if ($s1==="rc") $s1="-2";
		else if ($s1==="pre") $s1="-3";
		else if ($s1==="beta") $s1="-4";
		else if ($s1==="alpha") $s1="-5";
		else if ($s1==="prealpha") $s1="-6";
		else if ($s1==="git" || $s1==="svn" || $s1==="hg" || $s1==="r" || $s1==="rev" || $s1==="cvs") $s1="-7";

		if ($s2==="rc") $s2="-2";
		else if ($s2==="pre") $s2="-3";
		else if ($s2==="beta") $s2="-4";
		else if ($s2==="alpha") $s2="-5";
		else if ($s2==="prealpha") $s2="-6";
		else if ($s2==="git" || $s2==="svn" || $s2==="hg" || $s2==="r" || $s2==="rev" || $s1==="cvs") $s2="-7";
		

		//print "s1: [$s1]<br>";
		//print "s2: [$s2]<br>";
		if ($s1===$s2) {
			//print "$s1 equals $s2, continuing<br>\n";
			continue;
		}
		//else print "[$s1] is NOT EQUAL [$s2], look wtf<br>";


		if (intval($s1)>intval($s2)) {
			//print "$s1 greater than $s2, returning 1<br>\n";
			return 1;
		}
		else if (intval($s1)<intval($s2)) {
			//print "$s1 less than $s2, returning -1<br>\n";
			return -1;
		}
		else {
			if (intval($s1)===0 && $s1!=="0") {
				if (intval($s2)===0 && $s2!=="0") {
					$scomp = strcmp($s1, $s2);
					if ($scomp<0) {
						//print "$s1 &lt; $s2, Ret A<br>";
						return -1;
					}
					else if ($scomp>0) {
						//print "$s1 &gt; $s2, Ret B<br>";
						return 1;
					}
				}
				else {
					//print "Ret C<br>";
					return 1;
				}
			}
			else { // Means first is string zero
				if (intval($s2)===0 && $s2!=="0") {
					//print "$s1 &lt; $s2, Ret D<br>";
					return -1;
				}
			}
		}
	}
	//print "$s1 == $s2: Ret ZERO<br>";
	return 0;

}

// Condition should be in SQL format (int). Returns true if OK, false if not.
function checkDepCondition($required_version, $pkgversion, $condition) {
	$result = strverscmp($pkgversion, $required_version);
	switch($condition) {
	case 1:
		if ($result>0) return true;
		else return false;
	case 2:
		if ($result<0) return true;
		else return false;
	case 3: 
		if ($result==0) return true;
		else return false;
	case 4:
		if ($result!=0) return true;
		else return false;
	case 5:
		if ($result>=0) return true;
		else return false;
	case 6:
		if ($result<=0) return true;
		else return false;
	case 7:
		return true;
	default:
		print "Unknown condition $condition<br>";
		return false;
	}

}
function getDepCondition($cond) {
	switch($cond) {
		case 1: return '>';
		case 2: return '<';
		case 3: return '==';
		case 4: return '!=';
		case 5: return '>=';
		case 6: return '<=';
		case 7: return 'any';
	}
}

function getDepConditionFromXML($condition) {
	$condition = trim($condition);
	if ($condition=="more") return 1;
	if ($condition=="less") return 2;
	if ($condition=="equal") return 3;
	if ($condition=="notequal") return 4;
	if ($condition=="atleast") return 5;
	if ($condition=="notmore") return 6;
	if ($condition=="any") return 7;
	if ($condition=="(any)") return 7;
	return "OMG";
}
function getDepConditionBack($condition) {
	if ($condition==1) return "more";
	if ($condition==2) return "less";
	if ($condition==3) return "equal";
	if ($condition==4) return "notequal";
	if ($condition==5) return "atleast";
	if ($condition==6) return "notmore";
	if ($condition==7) return "any";
	return $condition;
}
?>
