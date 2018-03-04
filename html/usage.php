<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<title>WiPi-Heat: Usage Profile</title>
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
?>

 <div id="body">

<?php
#
#	WiPi-Heat MASTER Display Page
#
?>
    <h2>Usage Profile</h2>
    <p>
<?php
#    var_dump($_POST);
#
    $selected_date = html_select_date($menu_mode, $submenu_mode);
    $opmode = getWiPiopmode();
    if ($opmode == '-m') {
#
#	Get the Hours Run
#
	$hours_run = get_hours_run($hostname, $selected_date);
	echo "<br>Daily total of Hours Run: ", $hours_run, "<br>";
    }
?>
    </p>

<?php
    $zones = get_zone_names();
    $nodes = get_node_names();
    $nodezone = get_node_zone();
##
#	Generate Graph for all nodes/this node depending on node type
#
    if ($opmode == '-m') {
	for ($zone = 0; $zone < count($zones); $zone++) {
	    print("<h3>".$zones[$zone]."</h3>");
	    for ($node = 0; $node < count($nodes); $node++) {
		if($nodezone[$node] == $zones[$zone]) {
		    $filename = $nodes[$node].'.png';
?>
    <p>
    <?php if (file_exists($filename)) { unlink($filename);} ?>
    <?php generate_graph($hostname, $zone, $nodes[$node], $selected_date, 1); ?>
    <img src=<?php echo $filename ?> height=50% width=100%>
    </p>
<?php
		}
	    }
	}
    } else {
	$node = $hostname;
	$filename = $node.'.png';
?>
    <p>
    <?php if (file_exists($filename)) { unlink($filename);} ?>
    <?php generate_graph($hostname, $node, $selected_date, 1); ?>
    <img src=<?php echo $filename ?> height=50% width=100%>
    </p>

<?php
    }
?>
    <p>
    <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
    </p>
 </div>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
