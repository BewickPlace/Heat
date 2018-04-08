<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<?php
require 'html_functions.php';
require 'functions.php';
$hostname = getmyhostname();
print("<title>".$hostname.": About</title>");
?>
<link rel="stylesheet" type="text/css" href="style.css">
</head>

<body>
<div id="page">
 <div id="header">
<?php
#
#	Header section of the page
#
?>
 <h1> WiPi-Heat:  <?php echo $hostname ?></h1>
 </div>
 <p>
<?php
require 'manage_menu.php';
require 'manage_about_submenu.php';
?>
 </p>
 <div id="body">

<?php
#	var_dump[$_POST];
?>

<?php
#
#	Footer section of page
#
?>
  <h2>About the WiPi-Heat</h2>
  <p>
   Welcome to the configuration and set-up pages for the WiPi-Heat.  The WiPi-Heat is a sophisticated wireless heating Control system, that enables distributed sensors to control heat in Zones.
  </p>
  <p>
   This computer has the following components installed :
  <ul>
  <li>Raspbian operating system</li>
  <li>Samba file sharing</li>
  <li>Lighttpd & PHP Web Server</li>
  <li>WiringPi GPIO library</li>
  <li>JpGraph PHP driven charts</li>
  <li>Wi-Pi Heat Control System</li>
  </ul>
  </p>
  <p>
   This software is provided & licensed in accordance with the software licenses contained within this distribution.
  </p>
  <p>
   <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
  </p>
 </div>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
