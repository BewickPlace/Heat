<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<title>WiPi-Heat Temperature Control System</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>

<?php
require 'html_functions.php';
require 'functions.php';
?>

<body>
<div id="page">
 <div id="header">
<?php
#
#	Header section of the page
#
$hostname = getmyhostname();
?>
 <h1> WiPi-Heat:  <?php echo $hostname ?></h1>
 </div>

<?php
require 'manage_menu.php';
require 'manage_home_submenu.php';
?>

 <div id="body" align="center" width="%50">

<?php
#
#	WiPi-Heat MASTER Display Page
#
#
?>
    <h2>WiPi-Heat</h2>
    <h2>Temperature Control System</h2>

    <div id ="circle-plain" class="circle">
	Zone: <?php echo $hostname ?>
    </div>
    <p>
	Welcome to your WiPi-Heat.
    </p>
</div>
<div>
    <p>
    <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
    </p>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
