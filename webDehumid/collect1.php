<?php
#	Example of a "Collector" script.
#	IoT Device should POST a JSON string with any data.
#	This script prefaces the string with a time stamp and saves to 'realtm.txt'
#	KEEP THE FILENAME 8-characters !!!
	
	date_default_timezone_set("America/New_York");
	$senstm = date( "H:i:s");						// get the time
	$json   = file_get_contents('php://input');		// get the POST string

	$store = '{"time":"' . $senstm .'",' . substr( $json, 1);	// preface with time

	$filename = "realtm.txt";
	if( isset( $_GET ) )
	{
		$sensid  = $_GET["id"];							// get the name of the sensor
		if( $sensid !== '' )
			$filename = "RT-" . $sensid . ".txt";
	}
	
	file_put_contents( $filename, $store );				// save to file
#	file_put_contents( "logger.txt", $json, FILE_APPEND);

#	Pass back to the IoT Device whatever you want.
	$cmd = "command.txt";
	if( file_exists( $cmd ) )
	{
		$cnx = file_get_contents( $cmd );
		unlink( $cmd );
		echo $cnx;
	}
	else
	{
		echo "Saved to: " . $filename . " at " . $senstm;		
	}
/*
	You can decode the JSON string and manage each item individually,
	but this violates the principle of passing an opaque object.
	As an example,
		$data = json_decode($json);
		$sensid = $data->sensor;
		$sensval = $data->tempF . ", " . $data->humid;

		echo "Logged $senstm, " . $sensid . ", " . $sensval . "\n";
		echo "TEMPS: ";
		foreach( $data->temps as $t )
			echo $t . ", ";
		echo "\n";

	You can optionally use GET or POST of data not in JSON format.
	As an example, you can use Content-type app/encoded, and
		$sensid  = $_REQUEST["id"];	
		$sensval = $_REQUEST["val"];
		echo "Logged $senstm, " . $sensid . ", " . $sensval;
*/
?>

