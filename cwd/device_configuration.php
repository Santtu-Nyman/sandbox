<?php

if (isset($_POST['id']))
{
	$device_id = trim(filter_var($_POST['id'], FILTER_SANITIZE_STRING));
	$mode = '1';
	$measurement_period = '60';
	$target_temperature = '16';
	echo $device_id . ',' . $mode . ',' . $measurement_period . ',' . $target_temperature;
}
else
{
	echo '';
}

?>