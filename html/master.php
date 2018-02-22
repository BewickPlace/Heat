<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<title>WiPi-Heat Temperature Control System</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>

<?php
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
    <h2>Welcome to your WiPi-Heat Temperature Control System</h2>
    <p>
<?php
    $system = get_system_name();
    $zones = get_zone_names();
    $nodes = get_node_names();
?>
    </p>
    <p>
	Your system <?php echo $system[0] ?> contains <?php echo count($zones)?> Zones:
	<br>
    <?php foreach($zones as $zone) { echo $zone, "<br>"; } ?>
    </p>

    <h2>Current Usage Profiles</h2>
    <p>

<?php
#    var_dump($_POST);
#
    $time = time();
    $date = date('Y-m-d', $time);
    $selected_date = (!empty($_POST['graph_date']) ? $_POST['graph_date'] : $date);
?>

    <form  method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>" autocomplete="off">
    Select Date :
    <select name="graph_date">
<?php
    for($i = 1; $i <30; $i++) {
	$date =  date('Y-m-d', $time);
	$selected = ($date == $selected_date ? " selected" : "");
?>
	<option value=<?php echo $date, $selected;?>><?php echo $date;?></option>
<?php
	$time = $time -(60 *60*24);
    }
?>
    </select>
    <input type="submit" value="Produce Graph">
    </form>
<?php
#
#	Get the Hours Run
#
    $hours_run = get_hours_run($hostname, $selected_date);
    echo "<br>Daily total of Hours Run: ", $hours_run, "<br>";;
?>
    </p>

<?php
#
#	Generate Graph for this node
#
    foreach($nodes as $node) {
	$filename = $node.'.png';
?>
    <p>
    <?php unlink($filename); ?>
    <?php generate_graph($node, $selected_date); ?>
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
