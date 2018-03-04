<?php
#
#	Manage the Menu tabs consistently for all pages
#	Get menu mode eiether as a parameter or as hidden field on forms
#
$submenumode = "";
#
$class_diag = "";
$class_monitor = "";
$class_system = "";
$class_change = "";
$class_abouts = "";
#
$submenu_mode = (!empty($_GET['submenumode']) ? $_GET['submenumode'] : (!empty($_POST['submenuselect']) ? $_POST['submenuselect'] : ""));

switch($submenu_mode) {
case "diag":
    $logfile = $Heatlogfile;
    $class_diag = "current";
    break;

case "monitor":
    $logfile = $Monitorlogfile;
    $class_monitor = "current";
    break;

case "system":
    $logfile = $Dmesglogfile;
    $class_system = "current";
    break;

case "change":
    $class_change = "current";
    break;

case "abouts":
    $class_abouts = "current";
    break;

default:
    $class_abouts = "current";
    break;
}
?>

<ol id="toc1">
    <li class=<?php echo $class_abouts 	?>><a href="about.php?menumode=about&submenumode=abouts">		About</a></li>
    <li class=<?php echo $class_diag 	?>><a href="diagnostics.php?menumode=about&submenumode=diag">		Diagnostics</a></li>
    <li class=<?php echo $class_monitor	?>><a href="diagnostics.php?menumode=about&submenumode=monitor"> 	System Monitor</a></li>
    <li class=<?php echo $class_system	?>><a href="diagnostics.php?menumode=about&submenumode=system"> 	System Information</a></li>
    <li class=<?php echo $class_change  ?>><a href="changelog.php?menumode=about&submenumode=change">		Changelog</a></li>
</ol>
