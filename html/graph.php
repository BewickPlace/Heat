<?php // content="text/plain; charset=utf-8"
#
#	JpGraph Graphing functions
#
require_once ('jpgraph/jpgraph.php');
require_once ('jpgraph/jpgraph_bar.php');
require_once ('jpgraph/jpgraph_line.php');
#
#	Include this as not available within Jesiie PHP and required by JPGraph
#
function imageantialias($image, $enabled){
        return true;
    }
#
#       Generate Graph
#	type:
#	1 - usage profile
#	2 - ??
#	3 - ??
#
function generate_graph($node, $selected_date, $graph_type) {

    updatenetwork_share(FALSE, $node, 'Heat');
    system('mount  /mnt/network', $retval);
        // Open a known directory, and proceed to read its contents
    if ($retval == 0) {
        $logfile = '/mnt/network/'.$node.'_'.$selected_date.'.csv';

        if (file_exists($logfile)) {
            $csv = array_map('str_getcsv', file($logfile));

	    switch ($graph_type) {
	    case 2:
		operating_graph($node, array_column($csv,0), array_column($csv, 2), array_column($csv,3), array_column($csv,4));
		break;

	    case 1:
	    default:
		data_graph($node, array_column($csv,0), array_column($csv, 1), array_column($csv,2), array_column($csv,3));
		break;
	    }
        } else {
	    echo "<font color='Red'>".$node.": File not available - select alternate date <font color='Black'>", "<br><br>";
        }
    } else {
        echo "<font color='Red'>".$node.": No data currently accessible (", $retval, ")<font color='Black'>", "<br><br>";
    }

    system('umount /mnt/network');
    updatenetwork_share(TRUE, '', '');
}

#
#	Profile Graph - Graph a zone profile
#
function profile_graph($data){

}
#
#	Data_graph - graph data collected by sensors
#
function data_graph($node, $time, $temp, $setpoint, $boost) {

    setlocale (LC_ALL, 'et_EE.ISO-8859-1');

    array_shift($time);
    array_shift($setpoint);
    array_shift($boost);
    array_shift($temp);
    $data1y = $setpoint;
    $data2y = $boost;
    $data6y = $temp;

// Create the graph. These two calls are always required
    $graph = new Graph(1600,400);
    $graph->clearTheme();
    $graph->SetScale("textlin", 15, 23);
    $graph->SetClipping(TRUE);

    $graph->SetShadow();
    $graph->img->SetMargin(60,30,20,40);

// Create the bar plots
    $b1plot = new BarPlot($data1y);
    $b1plot->SetFillColor("orange");
    $b2plot = new BarPlot($data2y);
    $b2plot->SetFillColor("red");

// Create the grouped bar plot
    $gbplot = new AccBarPlot(array($b1plot,$b2plot));
    $gbplot->SetWidth(1.0);

// Create the Line plot
    $lplot = new LinePlot($data6y);

    $lplot->SetBarCenter();
    $lplot->SetColor("blue");
    $lplot->mark->SetType(MARK_UTRIANGLE,'',1.0);
    $lplot->mark->SetWeight(2);
    $lplot->mark->SetWidth(8);
    $lplot->mark->setColor("blue");
    $lplot->mark->setFillColor("blue");

// ...and add it to the graPH
    $graph->Add($gbplot);
    $graph->Add($lplot);

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
//$graph->SetScale('intlin');
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
