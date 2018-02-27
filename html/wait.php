<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<title>WiPi-Heat: Wait for shutdown/restart</title>
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
 <h1>WiPi Air</h1>
 </div>
 <ol id="toc">
    <li class="current"><a href="index.php" >Home</a></li>
    <li><a href="music.php" >Music Server</a></li>
    <li><a href="changelog.php">Changelog</a></li>
    <li><a href="diagnostics.php">Diagnostics</a></li>
    <li><a href="about.php">About</a></li>
 </ol>
 <div id="body">

<?php
#
#	WiPi-Air waiting page
#
?>
<h2>WiPi-Air Shutdown or Restart:</h2>
<div id="myProgress">
  <div id="myBar">0%</div>
</div>

<script>
var page = location.search.split('page=')[1];
var label = ' Shutting down...';
var timeout = 30;

function move() {
    var elem = document.getElementById("myBar");
    var width = 0;
    var inc = 1;
    var id = setInterval(frame, timeout * 10 * inc );
    function frame() {
        if (width >= 100) {
            clearInterval(id);
	    if (page) { window.location.href = page; }
        } else {
            width = width + inc;
            elem.style.width = width + '%';
	    if (width >= 100) {
		elem.innerHTML = label + ' complete';
	    } else {
		elem.innerHTML = width * 1 + '%' + label;
	    }
        }
    }
}

if (page) { 
    label = ' Restarting...';
    timeout = 2 * timeout;
}
move();
</script>

<?php
#
#	Footer section of page
#
?>
   <p>
   <small>Overall &copy IT and Media Services 2013-<?php echo date("y"); ?></small>
  </p>
 </div>
</div>
<!-- s:853e9a42efca88ae0dd1a83aeb215047 -->
</body>
</html>
