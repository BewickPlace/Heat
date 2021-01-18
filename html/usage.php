<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<?php
require 'html_functions.php';
require 'functions.php';
require 'graph.php';
$hostname = getmyhostname();
print("<title>".$hostname.": Usage Profile</title>");
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

<?php
require 'manage_menu.php';
$submenu_mode="";
?>

 <div id="body">

<?php
#
#	WiPi-Heat MASTER Display Page
#
#    var_dump($_POST);
#
?>
    <h2>Usage Profile</h2>
    <p>
<?php
    $opmode = getWiPiopmode();
    if ((!isset($_POST["graph_date"])) && ($opmode == '-m')) {

    $selected_date = html_select_date($menu_mode, $submenu_mode);
    $node = $hostname;
    $filename = $node.'.png';
    echo "<br>";

    $dates = array();
    $hours_run = array();
    $status_ok = array();
    $status_inc = array();
    $status_nod = array();
    $status = 0;
    $base = strtotime("midnight", time());

    for($i = 30; $i > 0 ; $i--) {
#
#	Get the Hours Run & form array for display
#
	$time = $base - (($i-1)*24*60*60);
	$date = date('Y-m-d', $time);
	$hours_run[30-$i] = time_to_decimal(get_hours_run($hostname, $date, $status));
//	$hours_run[30-$i] = 60*60*(int) get_hours_run($hostname, $date, $status);
//	$hours_run[30-$i] = $i*60;
//	echo $date, ">>", $hours_run[30-$i], "<br><br>";

        $dates[]	= $time;
	$status_ok[]	= ( $status==2 ? 1 : 0);
	$status_inc[]	= ( $status==1 ? 1 : 0);
	$status_nod[]	= ( $status==0 ? 1 : 0);
    }
#	Add blank cells
    $dates[] = $base + (24*60*60);
    $hours_run[] = 0;
    $status_ok[] = 0;
    $status_inc[]= 0;
    $status_nod[]= 0;

#    var_dump($dates);
#    var_dump($hours_run);
#    var_dump($status_ok);
#    var_dump($status_inc);
#    var_dump($status_nod);

    if (file_exists($filename)) { unlink($filename);}
#	Generate first graph - Hours Run per day
    if (generate_run_hours($hostname, $dates, $hours_run, $status_ok, $status_inc, $status_nod) == 0) {
     ?> <img src=<?php echo $filename ?> height=50% width=100%><?php
    }
?>
    </p>
    <p>
<?php
#	Generate second graph - Hours Run (avg) by month
    $filename = $node.'1.png';
    if (file_exists($filename)) { unlink($filename);}
    if (generate_run_hours_monthly($hostname, $selected_date) == 0) {
     ?> <img src=<?php echo $filename ?> height=50% width=100%><?php
    }

    } else {

    $selected_date = html_select_date($menu_mode, $submenu_mode);
    if ($opmode == '-m') {
#
#	Get the Hours Run
#
	$hours_run = get_hours_run($hostname, $selected_date, $status);
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
	    if(in_array($zones[$zone], $nodezone)) {
	    print("<h3>".$zones[$zone]."</h3>");
	    for ($node = 0; $node < count($nodes); $node++) {
		if($nodezone[$node] == $zones[$zone]) {
		    $filename = $nodes[$node].'.png';
?>
    <p>
<?php
    if (file_exists($filename)) { unlink($filename);}
    if (generate_graph($hostname, $zone, $nodes[$node], $selected_date, 1) == 0) {
     ?> <img src=<?php echo $filename ?> height=50% width=100%><?php
    }
?>
    </p>
<?php
		}
	    }
	    }
	}
    } else {
	$node = $hostname;
	$filename = $node.'.png';
?>
    <p>
<?php
    if (file_exists($filename)) { unlink($filename);}
    if (generate_graph($hostname, NULL, $node, $selected_date, 1) == 0) {
     ?> <img src=<?php echo $filename ?> height=50% width=100%><?php
    }
?>
    </p>

<?php
    }
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
