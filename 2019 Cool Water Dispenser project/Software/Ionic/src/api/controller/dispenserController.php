<?php
   error_reporting( E_ALL );
   ini_set( "display_errors", 1 );
   
   $paginaData = new stdClass();
   $paginaData->content = "";
   
   $dbhost = '35.205.58.185';
   $dbuser = 'default';
   $dbpass = 'default';
   $dbname = 'coolwaterdispenser';

   $dblink = new mysqli($dbhost, $dbuser, $dbpass, $dbname);

   include_once "../model/dispenserModel.php";
   
   $cwddb = new CwdDatabase($dblink);
   //$building = new CwdDatabase($dblink);
   
   $areaData = $cwddb->GetData();
   //$BuildingData = $cwddb->GetBuildingData();
    
   echo $areaData;
?>