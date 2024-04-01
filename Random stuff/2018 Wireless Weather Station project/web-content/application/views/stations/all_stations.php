<script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
<script type="text/javascript">
  google.charts.load('current', { 'packages': ['map'] });
  //google.charts.setOnLoadCallback(drawMap);
  function drawMap(coordinates, stationId) {
    var array = coordinates.split(",");
    var latitude = parseFloat(array[0]);
    var longitude = parseFloat(array[1]);
    var data = new google.visualization.DataTable();
      data.addColumn('number', 'Lat');
      data.addColumn('number', 'Long');
      data.addColumn('string', 'Name');
      data.addRow([latitude, longitude, stationId]);

    var options = {
      showTooltip: true,
      showInfoWindow: true,
      zoomLevel:11
    };
    var map = new google.visualization.Map(document.getElementById('map_div'));
    map.draw(data, options);
  };
</script>

<div id="main" style="width:80%;">
  <table class="dataframe" style="vertical-align: top; display:inline-block">
    <br>
    <caption>Stations</caption>
    <thread>
      <tr>
        <th>ID</th><th>Designation</th><th>Region</th><th>Description</th><th>Coordinates</th><th>Show on map</th>
      </tr>
    </thread>
    <tbody>
      <?php
         foreach ($a_stations as $s) {
           echo '<tr>';
           echo '<td>'.$s['idStation'].'</td>';
           echo '<td>'.$s['designation'].'</td>';
           echo '<td>'.$s['region'].'</td>';
           echo '<td>'.$s['description'].'</td>';
           echo '<td>'.$s['latitude'].','.$s['longitude'].'</td>';
           echo '<th> <button class="'.$s['latitude'].','.$s['longitude'].'" onclick="drawMap(this.className, this.id)">Show</button>';
           echo '</tr>';
         }
         ?>
    </tbody>
  </table>
<div id="map_div" style="display:inline-block"></div>
</div>
