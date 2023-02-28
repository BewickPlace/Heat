<?php // content="text/plain; charset=utf-8"
#
#	JpGraph Graphing functions
#
require_once ('jpgraph/jpgraph.php');
require_once ('jpgraph/jpgraph_bar.php');
require_once ('jpgraph/jpgraph_line.php');
require_once ('jpgraph/jpgraph_date.php');
require_once ('jpgraph/jpgraph_utils.inc.php');
#
#	Include this as not available within Jesiie PHP and required by JPGraph
#
#function imageantialias($image, $enabled){
#        return true;
#    }
#
#	Simplify Array
#
function simplify_array(&$data) {
    $i;

    for ($i = 0; $i < count($data); $i++) {
	if ($data[$i] > 0) { $data[$i] = 1; }
    }
}

#	Expand Array
#	Expand the source arrays into a single column
#	with timestamps acording to the first array
#
function expand_array($time1, $time2, $source) {
    $i = 1;
    $j = 1;
    $out = array("Header");
    $value = NULL;

    for ($i = 1; $i < count($time1); $i++) {

	if ($j < count($time2)) { $cmp = strcmp($time1[$i], $time2[$j]); }
	while(($cmp >= 0) && ($j < count($time2))) {
	    $value = $source[$j];

	    $j++;
	    if ($j < count($time2)) { $cmp = strcmp($time1[$i], $time2[$j]); }
	}
	$out[] = $value;
    }
    return($out);
}
#
#	Get Monthly Average
#
function get_monthly_avg($node, $year, $month) {
    $avg = 0;
    $count = 0;

    $logfile = '/mnt/storage/Heat/'.$node.'_'.$year.'.csv';
    if (file_exists($logfile)) {
	$csv = array_map('str_getcsv', file($logfile));
	$dates = array_column($csv, 0);
	$run_hours = array_column($csv, 1);
	array_shift($dates);
	array_shift($run_hours);

	$i = 0;
	foreach ($run_hours as $time) {
	    $match = strtok($dates[$i], "-");
	    $match = strtok("\n");

	    if($match == $month) {
		$minutes = strtok($time, ":") * 60;
		$minutes = $minutes + strtok("\n");
		$avg = $avg + $minutes;
		$count++;
	    }
	    $i++;
	}
    }
    $avg = round((($count > 0)? $avg/$count : 0), 0);
    $avg = $avg/60;
    return($avg);
}
#
#	Generate Run Hours Monthly Graph
#
function generate_run_hours_monthly($node, $selected_date) {
    $dates = array();
    $run_hours = array();

    $year = strtok($selected_date, "-");
    $month = strtok("-");

    $time = strtotime($selected_date);
    $base = date("Y-m-01", $time);

    if($month > 6) { $month = $month - 6; $year--; }
    else	   { $month = $month + 6; $year--; $year--;}

    for($i=0; $i<=18; $i++) {
	$date = $base." -".(18-$i)." month";
	$time = strtotime($date);

	$dates[] = $time;
	$run_hours[] = get_monthly_avg($node, $year, $month);

	$month = $month + 1;
	if($month > 12) { $month = 1; $year++; }
    }
    $date = $base." + 1 month";
    $time = strtotime($date);
    $dates[] = $time;
    $run_hours[] = 0;

// Now get labels at the start of each month
    $dateUtils = new DateScaleUtils();
    list($tickPositions,$minTickPositions) = $dateUtils->GetTicks($dates);

// We add some grace to the end of the X-axis scale so that the first and last
// data point isn't exactly at the very end or beginning of the scale
    $grace = 000000;
    $xmin = $dates[0]-$grace;
    $xmax = $dates[19]+$grace;

// Create the graph. These two calls are always required
    $graph = new Graph(1600,400);
    $graph->clearTheme();
    $graph->SetScale("intint", 0, 0, $xmin, $xmax );
    $graph->yaxis->scale->SetAutoMin(0);

    $graph->SetShadow();
    $graph->img->SetMargin(60,30,20,40);

// Create the bar plots
    $lplot = new LinePlot($run_hours, $dates);
    $lplot->SetStepStyle();
    $lplot->SetColor("orange");
    $lplot->SetFillColor("orange");

// ...and add it to the graPH
    $graph->Add($lplot);

    $title = "Average Run Hours per Month for 18 months";
    $graph->title->Set($title);
    $graph->xaxis->title->Set("Month");
    $graph->yaxis->title->Set("Hours");
    $graph->yaxis->SetTitleMargin(40);
    $graph->xaxis->SetPos('min');

// Now set the tic positions
    $graph->xaxis->SetTickPositions($tickPositions,$minTickPositions);
    $graph->xaxis->SetLabelFormatString('M',true);

    $graph->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->yaxis->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->xaxis->title->SetFont(FF_FONT1,FS_BOLD);

    $graph->SetTickDensity(TICKD_NORMAL, TICKD_VERYSPARSE);
    $graph->xaxis->scale->ticks->SupressLast(True);

// Display the graph to image file
    $filename=$node.'1.png';
    $graph->Stroke($filename);
    return(0);
}
#
#	Gerenrate Run Hours Graph
#
function generate_run_hours($node, $dates, $run_hours, $status_ok, $status_inc, $status_nod) {

// Create the graph. These two calls are always required
    $graph = new Graph(1600,400);
    $graph->clearTheme();
    $graph->SetScale("datint");
    $graph->yaxis->scale->SetAutoMin(0);

//    $graph->SetClipping(TRUE);

    $graph->SetShadow();
    $graph->img->SetMargin(60,30,20,40);

// Create the bar plots
    $lplot = new LinePlot($run_hours, $dates);
    $lplot->SetStepStyle();
    $lplot->SetColor("orange");
    $lplot->SetFillColor("orange");

// Create Controls Line plots
    $lplot2 = new LinePlot($status_ok, $dates);
    $lplot2->SetBarCenter();
    $lplot2->SetStepStyle();
    $lplot2->SetColor("green");
    $lplot2->SetFillColor("green");

    $lplot1 = new LinePlot($status_inc, $dates);
//    $lplot1->SetBarCenter();
    $lplot1->SetStepStyle();
    $lplot1->SetColor("blue");
    $lplot1->SetFillColor("blue");

    $lplot0 = new LinePlot($status_nod, $dates);
    $lplot0->SetStepStyle();
    $lplot0->SetColor("red");
    $lplot0->SetFillColor("red");

// ...and add it to the graPH
    $graph->Add($lplot);
    $graph->AddY(0,$lplot2);
    $graph->AddY(1,$lplot1);
    $graph->AddY(2,$lplot0);
//    $graph->Add($lplot);

    $graph->SetYscale(0,'lin', 0, 50);
    $graph->SetYscale(1,'lin', 0, 50);
    $graph->SetYscale(2,'lin', 0, 50);
    $graph->ynaxis[0]->Hide();
    $graph->ynaxis[1]->Hide();
    $graph->ynaxis[2]->Hide();

    $title = "Run Hours per Day for last month";
    $graph->title->Set($title);
    $graph->xaxis->title->Set("Days");
    $graph->xaxis->scale->SetDateFormat('d-M');
    $graph->xaxis->scale->SetDateAlign(DAYADJ_1);
    $graph->yaxis->title->Set("Hours");
    $graph->yaxis->SetTitleMargin(40);

    $graph->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->yaxis->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->xaxis->title->SetFont(FF_FONT1,FS_BOLD);

    $graph->SetTickDensity(TICKD_NORMAL, TICKD_VERYSPARSE);

    $graph->xaxis->scale->ticks->SupressMinorTickMarks(True);
    $graph->xaxis->scale->ticks->SupressLast(True);

// Display the graph to image file
    $filename=$node.'.png';
    $graph->Stroke($filename);

}
#
#       Generate Graph
#	type:
#	1 - usage profile
#	2 - control report
#	3 - ??
#
function generate_graph($host, $zone_id, $node, $selected_date, $graph_type) {

    updatenetwork_share(FALSE, $node, 'Heat');
    system('mount  /mnt/network', $retval);
        // Open a known directory, and proceed to read its contents
    if ($retval == 0) {
        $logfile = '/mnt/network/'.$node.'_'.$selected_date.'.csv';
        $confile = '/mnt/storage/Heat/'.$host.'_'.$selected_date.'.csv';
        if ((file_exists($confile)) &&
	    (strcmp($host, $node) != 0)) {
            $csv2 = array_map('str_getcsv', file($confile));
	} else {
	    $csv2 = NULL;
	}
        if (file_exists($logfile)) {
            $csv = array_map('str_getcsv', file($logfile));

	    switch ($graph_type) {
	    case 2:
		$time	= array_column($csv,0);
		$zone1	= array_column($csv,2);
		$zone2	= array_column($csv,3);
		$athome	= array_column($csv,4);
		operating_graph($node, $time, $zone1, $zone2, $athome);
		break;

	    case 1:
	    default:
		$time	= array_column($csv,0);
		$temp	= array_column($csv,1);
		$setpoint = array_column($csv,2);
		$boost	= array_column($csv,3);
		$time2	= array_column($csv2,0);
		$zoned  = expand_array($time, $time2, array_column($csv2,$zone_id+2));
		$athome = expand_array($time, $time2, array_column($csv2,5));
		data_graph($node, $time, $temp, $setpoint, $boost, $zoned, $athome);
		break;
	    }
	    $retval = 0;
        } else {
	    echo "<font color='Red'>".$node.": File not available - select alternate date <font color='Black'>", "<br><br>";
	    $retval = 2;
        }
    } else {
        echo "<font color='Red'>".$node.": No data currently accessible (", $retval, ")<font color='Black'>", "<br><br>";
	$retval = 1;
    }

    system('umount /mnt/network');
    updatenetwork_share(TRUE, '', '');
    return($retval);
}

#
#	Profile Graph - Graph a zone profile
#
function profile_graph($data){

}
#
#	Data_graph - graph data collected by sensors
#
function data_graph($node, $time, $temp, $setpoint, $boost, $zone, $athome) {

    setlocale (LC_ALL, 'et_EE.ISO-8859-1');

    array_shift($time);
    array_shift($setpoint);
    array_shift($boost);
    array_shift($temp);
    array_shift($zone);
    array_shift($athome);
    simplify_array($athome);

// Create the graph. These two calls are always required
    $graph = new Graph(1600,400);
    $graph->clearTheme();
    $graph->SetScale("textlin");
    if (max($setpoint) >= 5) {
	$graph->yaxis->scale->SetAutoMin(max($setpoint)*2/3);
	$graph->yaxis->scale->SetAutoMax(max(max($setpoint),max($temp))+0.5);

    } else {
	$graph->yaxis->scale->SetAutoMax(1.5);
	$graph->yaxis->scale->SetAutoMin(0);
    }

    $graph->SetClipping(TRUE);

    $graph->SetShadow();
    $graph->img->SetMargin(60,30,20,40);

// Create the bar plots
    $b1plot = new BarPlot($setpoint);
    $b1plot->SetFillColor("orange");
    $b2plot = new BarPlot($boost);
    $b2plot->SetFillColor("red");

// Create the grouped bar plot
    $gbplot = new AccBarPlot(array($b1plot,$b2plot));
    $gbplot->SetWidth(1.0);

// Create the Line plot
    $lplot = new LinePlot($temp);

    $lplot->SetBarCenter();
    $lplot->SetColor("blue");
    $lplot->mark->SetType(MARK_UTRIANGLE,'',1.0);
    $lplot->mark->SetWeight(2);
    $lplot->mark->SetWidth(8);
    $lplot->mark->setColor("blue");
    $lplot->mark->setFillColor("blue");

// Create Controls Line plots
    $lplot2 = new BarPlot($athome);
    $lplot2->SetColor("blue");
    $lplot2->SetFillColor("blue");
    $lplot2->SetWidth(1.0);

    $lplot3 = new BarPlot($zone);
    $lplot3->SetColor("red");
    $lplot3->SetFillColor("red");
    $lplot3->SetWidth(1.0);

// ...and add it to the graPH
    $graph->Add($gbplot);
    $graph->AddY(0,$lplot3);
    $graph->AddY(1,$lplot2);
    $graph->Add($lplot);

    $graph->SetYscale(0,'lin', 0, 25);
    $graph->SetYscale(1,'lin', 0, 50);
    $graph->ynaxis[0]->Hide();
    $graph->ynaxis[1]->Hide();

    $title = "Daily Profile v Demand (".$node. ")";
    $graph->title->Set($title);
    $graph->xaxis->title->Set("Time of Day");
    $graph->yaxis->title->Set("Temperature");
    $graph->yaxis->SetTitleMargin(40);

    $graph->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->yaxis->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->xaxis->title->SetFont(FF_FONT1,FS_BOLD);

    $graph->xaxis->SetTickLabels($time);
    $graph->xaxis->SetTextTickInterval(12);
    $graph->xaxis->SetTextLabelInterval(1);

// Display the graph to image file
    $filename=$node.'.png';
    $graph->Stroke($filename);
}
#
#	Operating_graph - graph data on controls collected by Master
#
function operating_graph($node, $time, $zone1, $zone2, $athome) {

    setlocale (LC_ALL, 'et_EE.ISO-8859-1');

    array_shift($time);
    array_shift($zone1);
    array_shift($zone2);
    array_shift($athome);

    if (end($time) !== "23:55") {
	array_push($time, strftime("%H:%M"));
	array_push($zone1, end($zone1));
	array_push($zone2, end($zone2));
	array_push($athome, end($athome));
    }

// Create the graph. These two calls are always required
    $graph = new Graph(1600,400);
    $graph->clearTheme();
//    $graph->SetScale("textlin", 15, 23);
    $graph->SetScale("textlin", 0, 1);
    $graph->SetYScale(0,"lin", 0, 1.5);
    $graph->SetYScale(1,"lin", 0, 3);
    $graph->SetClipping(TRUE);

    $graph->SetShadow();
    $graph->img->SetMargin(40,40,40,80);
// Create the Line plot
    $lplot1 = new linePlot($zone1);
    $lplot1->SetStepStyle();
    $lplot2 = new LinePlot($zone2);
    $lplot2->SetStepStyle();
    $lplot3 = new LinePlot($athome);
    $lplot3->SetStepStyle();

    $lplot1->SetColor("red");
    $lplot2->SetColor("orange");
    $lplot3->SetColor("blue");
    $lplot1->SetFillColor("red");
    $lplot2->SetFillColor("orange");
    $lplot3->SetFillColor("blue");

// ...and add it to the graPH
    $graph->Add($lplot3);
    $graph->AddY(0,$lplot2);
    $graph->AddY(1,$lplot1);

// Hide additional axis

    $title = "Daily Operating Controls (".$node. ")";
    $graph->title->Set($title);
    $graph->xaxis->title->Set("Time of Day");
    $graph->yaxis->title->Set("State");
    $graph->yaxis->SetTitleMargin(80);

    $graph->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->yaxis->title->SetFont(FF_FONT1,FS_BOLD);
    $graph->xaxis->title->SetFont(FF_FONT1,FS_BOLD);

// Format the y-axis to remove axis
    $graph->yaxis->Hide();
    $graph->ynaxis[0]->Hide();
    $graph->ynaxis[1]->Hide();

// add a legend
    $graph->legend->SetLayout(LEGEND_HOR);
    $graph->legend->Pos(0.5, 0.95, "center", "bottom");
    $lplot1->SetLegend("Zone 1");
    $lplot2->SetLegend("Zone 2");
    $lplot3->SetLegend("Bluetooth");
    $graph->legend->SetReverse();

    $graph->xaxis->SetTickLabels($time);
//    $graph->xaxis->SetTextTickInterval(12);
    $graph->xaxis->SetTextTickInterval(1);
    $graph->xaxis->SetTextLabelInterval(1);

// Display the graph to image file
    $filename=$node.'.png';
    $graph->Stroke($filename);
}
?>
