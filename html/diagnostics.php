<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<?php
require 'html_functions.php';
require 'functions.php';
$hostname = getmyhostname();
print("<title>".$hostname.": Diagnostics</title>");
?>
<link rel="stylesheet" type="text/css" href="style.css">
</head>

<?php
#
#	Key Parameters
#
$Heatlogfile = "/var/log/heat.log";
$Monitorlogfile = "/var/log/monitor.log";
$Dmesglogfile = "Dmesg";
$Displaylines = 30;
$class_heat = "";
$class_monitor = "";
$class_system = "";
?>

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
#
#	POST Handling
#
#	var_dump($_POST);
#	echo "<br><br>";
#
	  if (isset($_POST['submit'])) {
	  switch ($_POST["submit"]) {
	  case "Select Diagnostics":
#	   No extra functions to perform
	    break;

	  case "Refresh Display":
	    $Displaylines = test_input($_POST["displaylines"]);
	    break;

	  case "Delete Logfile":
#	    $logfile = ($Heatselect =="checked" ? $Heatlogfile : $logfile);
#	    $logfile = ($Shutdownselect  =="checked" ? $Shutdownlogfile  : $logfile);
	    if (unlink($logfile) == 0) {echo "<font color='red'>Delete (",$logfile,") failed - check permissions<font color='black'><br><br>";}
	    break;
	  }
	  }

	  if(isset($_POST['verbose'])) {
	  switch($_POST["verbose"])
	  {
          case "TRUE":
	    updateWiPidebug("-v");

	    break;
          case "FALSE":
	    updateWiPidebug("");

	    break;
	  }
	  }


?>
<?php
#
#
#
switch($submenu_mode) {
case "monitor":
?>
    <h2>WiPi-Heat System Monitor Diagnostics</h2>
    <p>

    No options currently available.
    <p>
<?php
    break;

case "system":
?>
    <h2>WiPi-Heat System Diagnostics</h2>
    <p>
<?php
	# Display disk usage
	#
        $df = disk_free_space("/root")/(1024*1024*1024);
        echo "System root partition disk usage : ", sprintf("%.1f", $df), "GB free", "<br><br>";

	# Display available networks
	#
        echo "Wireless networks scan:", "<br>";
        exec('iwlist wlan0 scan | grep -A 8 Address', $iwout, $iwret);
	echo "<pre>";
        foreach ($iwout as $value) { echo $value, "<br>"; }
	echo "</pre>";

    break;

default:
	$verbose = ((getWiPidebug() == "-v")? "checked":"unchecked");
?>
	<h2>WiPi-Heat  Diagnostics</h2>
	<p>

	<form  method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>" autocomplete="off">
	<input type="hidden" name="verbose" value="FALSE">
	Verbose diagnostics:        <input type="checkbox" name="verbose"   Value="TRUE" <?php echo $verbose   ?> onchange="this.form.submit()"> <br>
	<input type="hidden" name="menuselect" value=<?php echo $menu_mode ?>>
	<input type="hidden" name="submenuselect" value=<?php echo $submenu_mode ?>>
	</form>
	<p>
<?php
    break;
}

?>
<?php
#
#	Display the selected logfile if available
#
?>
	<h2>Logfile:</h2>
	<p>

	<form  method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>" autocomplete="off">
	Lines to display: <input type="text" name="displaylines" value=<?php echo $Displaylines ?> size=5 maxlength=3 pattern="[0-9]+" required title="Numeric">
	<input type="submit" name="submit" value="Refresh Display">
	<?php if ($logfile !== $Dmesglogfile) { ?> <input type="submit" name="submit" value="Delete Logfile"><?php } ?>
	<input type="hidden" name="menuselect" value=<?php echo $menu_mode ?>>
	<input type="hidden" name="submenuselect" value=<?php echo $submenu_mode ?>>
	</form>
       <p>
<?php
	echo $submenumode, " log file: ";
        if ($logfile == $Dmesglogfile) {
           $cmd = sprintf("dmesg -T | tail -n%s",$Displaylines);
        } else {
           $cmd = sprintf("tail -n%s %s",$Displaylines,$logfile);
        }
	exec($cmd , $tail, $ret);

	switch (count($tail))
	{
	case 0:
	  echo "No log file available", "<br>";
	  break;
	default:
	  echo "<br>", "<pre>";
	  foreach ($tail as $value) { echo $value, "<br>"; }
	  echo "</pre><br>";
	}

?>

<?php
#
#	Footer section of page
#
?>
	<form  method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>" autocomplete="off">
	<input type="submit" name="submit" value="Refresh Display"><br>
	<input type="hidden" name="displaylines" value=<?php echo $Displaylines ?>>
	<input type="hidden" name="menuselect" value=<?php echo $menu_mode ?>>
	<input type="hidden" name="submenuselect" value=<?php echo $submenu_mode ?>>
	</form>
  <br><br>
   <small>Overall &copy IT and Media Services 2018-<?php echo date("y"); ?></small>
  </p>
 </div>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
