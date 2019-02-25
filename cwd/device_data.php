<?php

if (isset($_POST['id']) && isset($_POST['pt']) && isset($_POST['it']))
{
	$device_id = trim(filter_var($_POST['id'], FILTER_SANITIZE_STRING));// device id
	$unix_timestamp = trim(filter_var($_POST['pt'], FILTER_SANITIZE_STRING));// unix time
	$information_type = trim(filter_var($_POST['it'], FILTER_SANITIZE_STRING));// type of message
	if ($information_type == '0')
	{
		// device startup message received
		echo 'device startup message received id = ' . $device_id . ' pt = ' . $unix_timestamp . ' it = ' . $information_type;
	}
	else if ($information_type == '1' && isset($_POST['wl']) && isset($_POST['wt']))
	{
		// periodic measurement message received
		$water_level = trim(filter_var($_POST['wl'], FILTER_SANITIZE_STRING));// level in %
		$water_temperature = trim(filter_var($_POST['wt'], FILTER_SANITIZE_STRING));// temperature in celsius
		echo 'periodic measurement message received id = ' . $device_id . ' pt = ' . $unix_timestamp . ' it = ' . $information_type .
		' wl = ' . $water_level . ' wt = ' . $water_temperature;
	}
	else if ($information_type == '2')
	{
		// bypassing message received
		echo 'bypassing message received id = ' . $device_id . ' pt = ' . $unix_timestamp . ' it = ' . $information_type;
	}
	else if ($information_type == '3' && isset($_POST['ot']))
	{
		// water order message received
		$order_type = trim(filter_var($_POST['ot'], FILTER_SANITIZE_STRING));// the order is integer that means something
		echo 'order message received id = ' . $device_id . ' pt = ' . $unix_timestamp . ' it = ' . $information_type . ' ot = ' . $order_type;
	}
	echo '';
}
else
{
	echo '';
}

?>