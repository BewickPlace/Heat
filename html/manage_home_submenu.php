<?php
#
#	Manage the Menu tabs consistently for all pages
#	Get menu mode eiether as a parameter or as hidden field on forms
#
$submenumode = "";
#
$class_home = "";
$class_usage = "";
$class_controls = "";
#
$submenu_mode = (!empty($_GET['submenumode']) ? $_GET['submenumode'] : (!empty($_POST['submenuselect']) ? $_POST['submenuselect'] : ""));

switch($submenu_mode) {
case "home":
    $class_home = "current";
    break;

case "usage":
    $class_usage = "current";
    break;

case "controls":
    $class_controls = "current";
    break;

default:
    $class_home = "current";
    break;
}
?>

<ol id="toc1">
    <li class=<?php echo $class_home 	?>><a href="master.php?menumode=home&submenumode=home">		Home</a></li>
    <li class=<?php echo $class_usage	?>><a href="usage.php?menumode=home&submenumode=usage">	 	Usage</a></li>
    <li class=<?php echo $class_controls?>><a href="usage.php?menumode=home&submenumode=controls"> 	Controls</a></li>
</ol>
