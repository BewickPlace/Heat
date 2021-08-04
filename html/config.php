<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<?php
require 'html_functions.php';
require 'functions.php';
$hostname = getmyhostname();
print("<title>".$hostname.": Configure Control System</title>");
?>
<link rel="stylesheet" type="text/css" href="style.css">
</head>

<?php
$opmode = getWiPiopmode();
if ($opmode == '-s')  {
    ?><script>window.location.href = "index.php";</script><?php
}
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

<?php
require 'manage_menu.php';
?>

 <?php
    $bluedev = get_bluetooth_info();
#
# var_dump($bluedev);
# var_dump($_POST);
#
#	Obtain Bluetooth configuration data from returned POST

    $change = FALSE;
    for($device = 0; $device < count($bluedev); $device++){
	if (isset($_POST[$device+1])) {
	    $entry = test_input($_POST[$device+1]);
	    $devicename = substr($bluedev[$device],0 ,strpos($bluedev[$device], ','));

	    $bluedev[$device] = $devicename.", ".$entry;
	    $change = TRUE;
	}
    }
    if ($change) { update_bluetooth_info($bluedev);}
 ?>

 <div>
 <h2>Bluetooth Configuration:</h2>
 <p>
    <?php
    if ($change)  echo "<font color='Red'>Configuration Updated...<font color='Black'>", "<br><br>";
    ?>

    <form  method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>" autocomplete="off">
    <input type="hidden" name="menuselect" value=<?php echo $menu_mode ?>>
    <table style=\"width:17%; text-align:center\">
    <tr>
	<th>Device</th>
	<th>Address</th>
    </tr>

    <?php
    for($device = 1; $device <(count($bluedev)+1); $device++) {
    ?>
	<tr>
	    <?php
	    $devicename = substr($bluedev[$device-1],0 ,strpos($bluedev[$device-1], ','));
	    $deviceaddr = substr($bluedev[$device-1], strpos($bluedev[$device-1], ',') + 1);
	    ?>
	    <td align=\"left\"><?php echo "$devicename" ?></td>
	    <td>
	    <input type="text" name=<?php echo "$device" ?> value=<?php echo "$deviceaddr"?> id=<?php echo "$device"?>  size=18 maxlength=17 pattern="^([0-9A-F]{2}[:-]){5}([0-9A-F]{2})$"  title="MAC address" onchange="this.form.submit()">
	    </td>
	</tr>
	<?php
    }
    ?>
    </table>
    </form>

 </div>
 <div id="body">

    <form  method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>" autocomplete="off">
    <input type="submit" name="submit" value="Refresh Display">
    <input type="hidden" name="menuselect" value=<?php echo $menu_mode ?>>
    </form>

	<h2>Visible Devices:</h2>
  <?php
$opmode = getWiPiopmode();

	$detail = "";
	$cmd = "hcitool scan";
	exec($cmd , $detail, $ret);

	switch (count($detail))
	{
	case 0:
	  echo "No configuration file available", "<br>";
	  break;
	default:
	  echo "<br>", "<pre>";
	  foreach ($detail as $value) { echo $value, "<br>"; }
	  echo "</pre><br>";
	}
 ?>

	<h2>Configuration file:</h2>
  <?php
	$detail = "";
	$cmd = "cat /etc/heating.conf";
	exec($cmd , $detail, $ret);

	switch (count($detail))
	{
	case 0:
	  echo "No configuration file available", "<br>";
	  break;
	default:
	  echo "<br>", "<pre>";
	  foreach ($detail as $value) { echo $value, "<br>"; }
	  echo "</pre><br>";
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
