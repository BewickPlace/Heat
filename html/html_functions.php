<?php
#
#	WiPi-Heat Functions with embedded html used for standard features:
#
#	- html_select_date
#
function html_select_date($menu, $submenu) {

    $time = time();
    $date = date('Y-m-d', $time);
    $selected_date = (!empty($_POST['graph_date']) ? $_POST['graph_date'] : $date);

    print('<form  method="post" action='.htmlspecialchars($_SERVER["PHP_SELF"]).' autocomplete="off">');
    print('Select Date: ');
    print('<select name="graph_date">');

    for($i = 1; $i <30; $i++) {
	$date =  date('Y-m-d', $time);
	$selected = ($date == $selected_date ? " selected" : "");

	print('<option value='.$date.' '.$selected.'>'.$date.'</option>');
	$time = $time -(60 *60*24);
    }
    print('</select>');
    print('<input type="submit" value="Produce Graph">');
    print('<input type="hidden" name="menuselect" value='.$menu.'>');
    print('<input type="hidden" name="submenuselect" value='.$submenu.'>');
    print('</form>');

    return($selected_date);
}

#
#
#
?>
