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
print("<title>".$hostname.": Temperature Control System</title>");
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
?>

 <div id="body" align="center" width="%50">

<?php
#
#	WiPi-Heat MASTER Display Page
#
#
    $time = time();
    $date = date('Y-m-d', $time);
    $year = strtok($date, "-");
    $month = strtok("-");

    $system = get_system_name();
    $zones = get_zone_names();
    $nodes = get_node_names();
    $nodezone = get_node_zone();
    $bluedev = get_bluetooth_info();
?>
    <h2><?php echo $system[0]?></h2>
    <h2>Temperature Control System</h2>
    <div>
<?php
    $hours_run = get_hours_run($hostname, $date, $status);
    echo "Daily total of Hours Run: ", $hours_run,"<br>";

    $hours_run = round(get_monthly_avg($hostname, $year, $month),1);
    echo "Average daily usage this month: ", $hours_run, "Kwh<br><br>";
?>
    </div>
    <div>
Last state change:
    <?php echo get_logtime($hostname, $date),"<br><br>"; ?>
    </div>
    <div>
Bluetooth At Home:
    <?php $at_home = get_logstate_at_home($hostname, $date);
    if ($at_home  == 0) {
	echo " FALSE";
    } else {
	echo " TRUE";
    } ?>
    </div>
    <div>
    <?php
    print("<table style=\"width:17%; text-align:center\">");
    print("<tr>");
	print("<th>Device</th>");
	print("<th>Status</th>");
    print("</tr>");

    $mask = 1;
    for($device = 0; $device <(5+1); $device++) {
	print("<tr>");
	    if ($device == 0) {
		print("<td align=\"left\">Override</td>");
	    } else {
		$devicename = substr($bluedev[$device-1],0 ,strpos($bluedev[$device-1], ','));
		print("<td align=\"left\">$devicename</td>");
	    }
	    if(($at_home & $mask)== 0) {
		print("<td>-</td>");
	    } else {
		print("<td>At Home</td>");
	    }
	print("</tr>");
	$mask = $mask << 1;
    }
    print("</table>");
    ?>

    </div>

    <?php $netstate = get_logstate_network($hostname, $date); ?>

    <div class="container">
    <?php for($i = 0; $i < count($zones); $i++) { ?>
    <?php if(in_array($zones[$i],$nodezone)) { ?>
    <?php if (get_logstate_zone($hostname, $date, $i) == 0) { ?>
	<div id ="circle-plain" class="circle">
    <?php } else { ?>
	<div id ="circle-border" class="circle">
    <?php } ?>
Zone: <?php echo $zones[$i] ?>
    </div>
    <?php } ?>
    <?php } ?>
    </div>

    <div class=container>
    <?php $z = 0; ?>
    <?php foreach($zones as $zone) { ?>

    <?php $found = 0; ?>
    <?php for($i = 0; $i < count($nodes); $i++) { ?>
    <?php if($nodezone[$i] == $zone) { ?>
	<?php if ($netstate & (((1 << ($z*4)) <<  $found))) { ?>
	    <div id="circle-zone-up" class="circle">
	<?php } else { ?>
	    <div id="circle-zone-down" class="circle">
	<?php } ?>
	<?php echo '<a href="http://' . $nodes[$i] . '.local" target="_blank">' . $nodes[$i] . '</a>'; ?>
        </div>
	<?php $found++; ?>
    <?php } ?>
    <?php } ?>
    <?php $z++; ?>
    <?php } ?>
    </div>



 </div>
    <p>
    <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
    </p>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
