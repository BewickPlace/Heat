<?php
#
#
#	This file contains the common Utility  functions required for
#	the WiPi-Heat web front end file and parameter handling
#
#
function test_input($data)
{
#
#	Input validaton function
#
$data = trim($data);
$data = stripslashes($data);
$data = htmlspecialchars($data);
return $data;
}

function extractstring($string,$key)
{
#
#	Extract the element in quotes after the defined key
#	Extract looking for newline if quotes not found
#
  $o = strpos($string, $key);
  $o = $o + strlen($key);
  $x = strpos($string, '"', $o)+1;
  if ($x > $o) {
    $y = strpos($string, '"', $x);
  } else {
    $x = $o;
    $y = strpos($string, "\n", $x);
  }

return substr($string, $x, $y-$x);
}

function updatestring($string,$key,$name)
{
#
#	Update the element in quotes after the defined key
#	Update looking for newline if quotes not found
#
  $o = strpos($string, $key);
  $x = strpos($string, '"', $o)+1;
  if ($x > $o) {
    $y = strpos($string, '"', $x);
  } else {
    $x = $o + strlen($key);
    $y = strpos($string, "\n", $x);
  }

return substr_replace($string, $name, $x, $y-$x);
}

function updatekey($string, $key1, $key2)
{
#
#	Update and replace the key with the alternate key
#
  $x = strpos($string, $key1);
  $l = strlen($key1);

return substr_replace($string, $key2, $x, $l);
}

function display_signal($input)
{
#
#	Extract the Signal level from an iwconfig result
#	If not found don't output anything
#
$key0 = 'Link Quality';
$key1 = 'Signal level';
$key2 = 'dBm';
$key3 = '/100';
$quality = 0;
$x = $y = $z =0;

#	Identify line 4 or 5 for Signal information
$idx = 4;
$x = strpos($input[$idx], $key1);
if (!$x) {
   $idx = 5;
   $x = strpos($input[$idx], $key1);
   if (!$x) return; 
}

#	Extract Link Quality
$linkqual = 0;
$x = strpos($input[$idx], $key0);
if ($x) {
   $x = $x + strlen($key0)+1;
   $y =strpos($input[$idx], $key3, $x);
   if ($y) {
      $s = substr($input[$idx], $x, $y-$x);
      $linkqual = intval($s);
   }
}

#	Extract Signal Level in dBm or /100
$sigdbm = 0;
$x = strpos($input[$idx], $key1);
$x = $x + strlen($key1)+1;
$y =strpos($input[$idx], $key2, $x);
$z =strpos($input[$idx], $key3, $x);
if (($z) and ((!$y) OR ($z < $y))) {
   $quality = 1;
   $y = $z;
}

#	If signal level found
if (($y) or ($z)) {
   $s = substr($input[$idx], $x, $y-$x);
   $sigdbm = intval($s);
}
#
# However if measure is quality (x/100) based convert to dBm using:
#	dBm = (Quality/2) - 100
#
if ($quality) {
    $sigdbm = ($sigdbm / 2) - 100;
}
#
# Driver quality is a mix of algorithms and often over stated relative to dBm
# back calculate from the dBm figure
#
$linkqual = (($sigdbm + 100) * 2);
#
if (($linkqual ==0) or ($sigdbm == 0))	{ echo "<font color='Red'><b>No Signal (",	$sigdbm, $key2, ")</b><font color='Black'>"; }
elseif ($linkqual < 55) 			{ echo "<font color='Red'><b>Poor Signal  (",    $sigdbm, $key2, ")</b><font color='Black'>"; }
elseif ($linkqual < 65) 			{ echo "<font color='Orange'><b>Fair Signal  (",	$sigdbm, $key2, ")</b><font color='Black'>"; }
elseif ($linkqual < 75) 			{ echo "<font color='Green'><b>Good Signal  (",	$sigdbm, $key2, ")</b><font color='Black'>"; }
else                   			{ echo "<font color='Green'><b>Excellent Signal  (", $sigdbm, $key2, ")</b><font color='Black'>"; }
return;
}

function getmykey($file, $primekey)
{
#
#	Get the WiPi-Air Key from a file
#
  $filedata = file_get_contents($file);
  if ($filedata !== FALSE)
  {
    $position = strpos($filedata, $primekey);
    if ($position !== FALSE)
    {
      $output = extractstring($filedata,$primekey);
    }
    else
    {
      $output = "";
    }
  }
  else
  {
  $output = FALSE;
  }
  return $output;
}

function updatemykey($file, $primekey, $name)
{
#
#	Update the WiPi-Air Key in a file
#
  if(getmykey($file, $primekey) !== $name)
  {
    $filedata = file_get_contents($file);
    if ($filedata !== FALSE)
    {
      $output = updatestring($filedata,$primekey,$name);
      if (file_put_contents($file, $output) === FALSE)
      {
#       Write error
        echo "<font color='Red'>Write failed - check permissions<font color='Black'>", "<br><br>";
        return(FALSE);
      }
    } else {
#     Read error
      return(FALSE);
    }
    return (TRUE);
  }
  else
  {
    return(FALSE);
  }
}


