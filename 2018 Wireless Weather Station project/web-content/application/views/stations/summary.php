<div id="main">
  <div class="space-out">
    <div style="text-align: center">
      <h4>Number of stations</h4>
      <?php echo '<h1>'.$station_count.'</h1>'?>
    </div>
    <div style="text-align: center">
    <h4>Measurements done</h4>
      <?php echo '<h1>'.$measurement_count.'</h1>'?>
    </div>
  </div>
  <div>
    <table id="latest" class="dataframe">
      <caption><h3> Latest readings </h3></caption>
      <thread>
        <tr>
          <th>Time</th><th>Station</th><th>Region</th><th>Temperature</th><th>Humidity</th><th>Pressure</th><th>Illuminance</th><th>CO</th>
        </tr>
      </thread>
      <tbody>
        <?php
           foreach ($measurements as $m) {
             echo '<tr>';
             echo '<td>'.$m['timestamp'].'</td>';
             echo '<td>'.$m['designation'].'</td>';
             echo '<td>'.$m['region'].'</td>';
             echo '<td>'.$m['temperature'].'</td>';
             echo '<td>'.$m['humidity'].'</td>';
             echo '<td>'.$m['pressure'].'</td>';
             echo '<td>'.$m['illuminance'].'</td>';
             echo '<td>'.$m['cmonoxide'].'</td>';
             echo '</tr>';
           }
        ?>
      </tbody>
    </table>
  </div>
</div>
