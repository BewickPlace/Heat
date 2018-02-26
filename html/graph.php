<?php // content="text/plain; charset=utf-8"
#
#	JpGraph Graphing functions
#
require_once ('jpgraph/jpgraph.php');
require_once ('jpgraph/jpgraph_bar.php');
require_once ('jpgraph/jpgraph_line.php');
#
#
#       Generate Graph
#
function generate_graph($node, $selected_date) {

    updatenetwork_share(FALSE, $node, 'Heat');
    system('mount  /mnt/network', $retval);
        // Open a known directory, and proceed to read its contents
    if ($retval == 0) {
        $logfile = '/mnt/network/'.$node.'_'.$selected_date.'.csv';

        if (file_exists($logfile)) {
            $csv = array_map('str_getcsv', file($logfile));
            data_graph($node, array_column($csv,0), array_column($csv, 1), array_column($csv,2), array_column($csv,3));

        } else {
	    echo "<font color='Red'>".$node.": File not available - select alternate date <font color='Black'>", "<br><br>";
        }
    } else {
        echo "<font color='Red'>".$node.": No data currently accessible (", $retval, ")<font color='Black'>", "<br><br>";
    }

    system('umount /mnt/network');
//    updatenetwork_share(TRUE, '', '');
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

//    $data1y = array(20,20,20,21,20,20);
//    $data2y = array(05,02,05,02,05,05);
//    $data6y = array(21,19,21,19,23,16);;

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
?>
