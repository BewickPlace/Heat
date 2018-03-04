<?php
#
#	Manage the Menu tabs consistently for all pages
#	Get menu mode eiether as a parameter or as hidden field on forms
#
$class_home = "";
$class_usage = "";
$class_config = "";
$class_device = "";
$class_about = "";

$menu_mode = (!empty($_GET['menumode']) ? $_GET['menumode'] : (!empty($_POST['menuselect']) ? $_POST['menuselect'] : ""));

switch($menu_mode)
{
case "home":
    $class_home = "current";
    break;

case "usage":
    $class_usage = "current";
    break;

case "config":
    $class_config = "current";
    break;

case "device":
    $class_device = "current";
    break;

case "about":
    $class_about = "current";
    break;

default:
    $class_home = "current";
    break;
}
$opmode = getWiPiopmode();
?>

<ol id="toc">
    <li class=<?php echo $class_home 	?>><a href="index.php?menumode=home">		Home</a></li>
    <li class=<?php echo $class_usage 	?>><a href="usage.php?menumode=usage">		Usage</a></li>
    <li class=<?php echo $class_device  ?>><a href="network.php?menumode=device">	Device</a></li>
<?php
    if ($opmode == "-m") {
?>
    <li class=<?php echo $class_config  ?>><a href="config.php?menumode=config">	Configuration</a></li>
<?php
    }
?>
    <li class=<?php echo $class_about   ?>><a href="about.php?menumode=about">		About</a></li>
</ol>
