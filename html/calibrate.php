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
print("<title>".$hostname.": Calibration</title>");
?>
<title>WiPi-Heat: Calibration</title>
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
    $opmode = getWiPiopmode();
require 'manage_menu.php';
require 'manage_about_submenu.php';
?>

 <div id="body">

<?php
#
#	WiPi-Heat MASTER Calibration Page
#
?>
    <h2>Profile Delta Calibration</h2>
    <p>
<?php
#    var_dump($_POST);
#
    $selected_date = html_select_date($menu_mode, $submenu_mode);
?>
    </p>
    <p>

<?php
#
#	Extract Time and Temperature arrays from CSV file
#
function get_node_value($node, $selected_date, &$time, &$temp) {

    updatenetwork_share(FALSE, $node, 'Heat');
    system('mount  /mnt/network', $retval);
        // Open a known directory, and proceed to read its contents
    if ($retval == 0) {
        $logfile = '/mnt/network/'.$node.'_'.$selected_date.'.csv';
        if (file_exists($logfile)) {
            $csv = array_map('str_getcsv', file($logfile));
	    $time = array_column($csv,0);
	    $temp = array_column($csv,1);

	    $retval = 0;
        } else {
//	    echo "<font color='Red'>".$node.": File not available - select alternate date <font color='Black'>", "<br><br>";
	    $retval = 2;
        }
    } else {
//      echo "<font color='Red'>".$node.": No data currently accessible (", $retval, ")<font color='Black'>", "<br><br>";
	$retval = 1;
    }
    system('umount /mnt/network');
    updatenetwork_share(TRUE, '', '');
    return($retval);
}
#
#	Calculate the variance between the baseline & the source
#
function recommend_calibration($node_baseline, $node_calibrate, $selected_date) {

    $variance = 0;
    if (get_node_value($node_baseline,  $selected_date, $basetime, $basetemp) != 0) { return(""); }
    if (get_node_value($node_calibrate, $selected_date, $nodetime, $nodetemp) != 0) { return(""); }
    expand_array($basetime, $nodetime, $nodetemp);

    for ($i = 1; $i < count($basetime); $i++) {
	$variance = $variance +  $nodetemp[$i] - ($basetemp[$i]); 
    }
    $variance = $variance / (count($basetime)-1);
    return($variance);
}

#
#	Main Calibration page
#
    $zones = get_zone_names();
    $nodes = get_node_names();
    $nodezone = get_node_zone();
    $deltas = get_node_profiledelta();
#
#	Generate Table for all Zones/Nodes
#
    for ($zone = 0; $zone < count($zones); $zone++) {
	$first = -1;
	print("<h3>".$zones[$zone]."</h3>");
	print("<table style=\"width: 30%; text-align:center;\">");
	print("<tr>");
	    print("<th>Node</th>");
	    print("<th>Current</th>");
	    print("<th>Recommended</th>");
	print("</tr>");
	for ($node = 0; $node < count($nodes); $node++) {
	if($nodezone[$node] == $zones[$zone]) {
	    print("<tr>");
	    print(sprintf("<td> %12s</td>", $nodes[$node]));
	    print("<td>$deltas[$node]</td>");
	    if ($first < 0) {
		$first = $node;
		print("<td>Baseline</td>");
	    } else {
		$variance = recommend_calibration($nodes[$first], $nodes[$node], $selected_date);
		if ($variance != "") {print(sprintf("<td>%.1f</td>", $variance));}
	    }
	    print("</tr>");
	}
	}
	print("</table>");
    }
?>
    </p>

    <p>
    <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
    </p>
 </div>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
