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

 <div id="body">
  <?php
$opmode = getWiPiopmode();

echo "Page under construstion";

#	Obtain configuration data from files or returned POST


#  var_dump($_POST);

    if ($_SERVER["REQUEST_METHOD"] == "POST") {
	switch ($_POST["submit"]) {
    	case "Login":
    	    $host     = test_input($_POST["host"]);
	    break;
	}
    }
    switch ($_POST["daapdenabled"]) {
    case "TRUE":
      enabledaapd(TRUE);
	 break;
    case "FALSE":
      enabledaapd(FALSE);
	 break;
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
