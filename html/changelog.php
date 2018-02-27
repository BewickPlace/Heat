<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<title>WiPi-Heat: Changelog</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>

<?php
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
require 'manage_about_submenu.php';
?>

 <p>

 <div id="body">
 <div style="line-height: 80%">

  <?php
  echo exec("uname -a"), "<br>"
  ?>
  <pre><?php
#
#	Display the chagelog if available
#
#
   echo file_get_contents("changelog.txt");
   ?></pre>
</div>
<?php
#
#	Footer section of page
#
?>
  <h2></h2>
  <p>
   <pre></pre>
   <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
  </p>
 </div>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
