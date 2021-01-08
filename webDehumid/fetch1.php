<?php
	#	Read data from the file "realtm.txt" (single measurement) and pass back

	$filename = "realtm.txt";
	if( isset( $_GET ) )
	{
		$sensid  = $_GET["id"];							// get the name of the sensor
		if( $sensid !== '' )
			$filename = "RT-" . $sensid . ".txt";
	}
	
	$cnx = file_get_contents( $filename );				// this contains a JSON object
	echo $cnx;
?>
