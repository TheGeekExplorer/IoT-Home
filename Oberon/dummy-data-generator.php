<?php

$lengthh = 24;
$lengthm = 60;
$n       = 28.0;
$c       = 0.0025;
$l       = "";
$pp      = 0;


for ($i = 0;  $i<($lengthh * $lengthm);  $i++) {
	$p = rand(($n-(rand(0,25)/10)),($n+(rand(0,25)/10)));
	$d = rand(0, 9);
	$p = "$p.$d";
	$n = $n - $c;
	$p = round(($pp + $p) / 2, 1);
	$l .= "$p,";
	$pp = $p;
}

$p = rtrim($p, ",");
file_put_contents("data.csv", $l);



