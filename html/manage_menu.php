<?php
#
#	Manage the Menu tabs consistently for all pages
#	Get menu mode eiether as a parameter or as hidden field on forms
#
$class_home = "";
$class_config = "";
$class_network = "";
$class_about = "";

$menu_mode = (!empty($_GET['menumode']) ? $_GET['menumode'] : (!empty($_POST['menuselect']) ? $_POST['menuselect'] : ""));

switch($menu_mode)
{
case "home":
    $class_home = "current";
    break;

case "config":
    $class_config = "current";
    break;

case "network":
    $class_network = "current";
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
<?php
    if ($opmode == "-m") {
?>
    <li class=<?php echo $class_config  ?>><a href="config.php?menumode=config">	Configuration</a></li>
<?php
    }
?>
    <li class=<?php echo $class_network ?>><a href="network.php?menumode=network">	Network</a></li>
    <li class=<?php echo $class_about   ?>><a href="about.php?menumode=about">		About</a></li>
</ol>
