<?php

if (isset($_POST['id']))
{
	$device_id = trim(filter_var($_POST['id'], FILTER_SANITIZE_STRING));// device id
	$mode = '1';// 1 normal operating mode and 0 off
	$measurement_period = '60';// time in seconds
	$target_temperature = '16';// temperature in celsius
	echo $device_id . ',' . $mode . ',' . $measurement_period . ',' . $target_temperature;
}
else
{
	echo '';
}

?>