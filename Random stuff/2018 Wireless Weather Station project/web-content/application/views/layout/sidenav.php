<div id="sidenavWrapper">
  <div id="sidenav">
    <a class="<?php if ($selected == "stations_summary") echo 'active'; ?>"   href="<?php echo site_url('Stations/stations_summary')?>"> Home </a>
    <a class="<?php if ($selected == "show_all_stations") echo 'active'; ?>"  href="<?php echo site_url('Stations/show_all_stations')?>"> All stations </a>
    <a class="<?php if ($selected == "station_search") echo 'active'; ?>"     href="<?php echo site_url('Stations/station_search')?>"> Station search </a>
    <a class="<?php if ($selected == "visualization") echo 'active'; ?>"      href="<?php echo site_url('Stations/data_visualization')?>"> Visualization </a>
    <br>
    <a class="<?php if ($selected == "station_management") echo 'active'; ?>" href="<?php echo site_url('Management/station_management')?>"> Station management </a>
  </div>
</div>
