<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<title>WiPi-Heat: Reports</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>

<?php
require 'html_functions.php';
require 'functions.php';
require 'graph.php';
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

 <div id="body">

<?php
#
#	WiPi-Heat MASTER Reports Display Page
#
?>
    <h2>Operational Reports</h2>
    <p>
<?php
#    var_dump($_POST);
#
    $selected_date = html_select_date($menu_mode, $submenu_mode);
    $opmode = getWiPiopmode();
?>
    </p>

<?php
#
#	Generate Graph
#
    $node = $hostname;
    $filename = $node.'.png';
echo $filename,"<br>";
?>
    <p>
    <?php if (file_exists($filename)) { unlink($filename);} ?>
    <?php generate_graph($hostname, NULL, $node, $selected_date, 2); ?>
    <img src=<?php echo $filename ?> height=50% width=100%>
    </p>

    <p>
    <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
    </p>
 </div>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
