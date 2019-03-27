<div id="main">
  <h3> Station management </h3>
  <?php echo "<span style=color:yellow>".validation_errors().$this->session->flashdata('error')."</span>"?>

  <div class="tab-container">
    <div class="tab">
      <input type="radio" id="tab-1" name="tab-group1" checked>
      <label for="tab-1">Add station</label>
      <div class="tab-content">
        <!-- ADD -->
        <form action="<?php echo site_url('Management/station_add'); ?>" method="post">
         <table>
             <tr><td>Designation</td><td><input type="text" name="designation" required></td></tr>
             <tr><td>Region</td><td><input type="text" name="region" required></td></tr>
             <tr><td>Interval</td><td><input type="text" name="interval" size=10 required></td></tr>
             <tr><td>Latitude</td><td><input type="text" name="latitude" size=10></td></tr>
             <tr><td>Longitude</td><td><input type="text" name="longitude" size=10></td></tr>
             <tr><td>Description</td><td><input type="text" name="description" size=37></td></tr>
             <tr><td></td><td><input type="submit" value="Add"></td></tr>
         </table>
        </form>
      </div>
    </div>

    <div class="tab">
      <input type="radio" id="tab-2" name="tab-group1">
      <label for="tab-2">Modify station</label>
      <div class="tab-content">
        <!-- MODIFY -->
        <form action="<?php echo site_url('Management/station_modify'); ?>" method="post">
        </br>
          Id to modify <input type="text" name="id" size=5 required></br>
          </br>
          To preserve old value, leave field empty.
         <table>
             <tr><td>Designation</td><td><input type="text" name="designation"></td></tr>
             <tr><td>Region</td><td><input type="text" name="region"></td></tr>
             <tr><td>Interval</td><td><input type="text" name="interval" size=10></td></tr>
             <tr><td>Latitude</td><td><input type="text" name="latitude" size=10></td></tr>
             <tr><td>Longitude</td><td><input type="text" name="longitude" size=10></td></tr>
             <tr><td>Description</td><td><input type="text" name="description" size=37></td></tr>
             <tr><td></td><td><input type="submit" value="Modify"></td></tr>
         </table>
        </form>
      </div>
    </div>

    <div class="tab">
      <input type="radio" id="tab-3" name="tab-group1">
      <label for="tab-3">Remove station</label>
      <div class="tab-content">
        <!-- REMOVE -->
        <form action="<?php echo site_url('Management/station_remove'); ?>" method="post">
          <table>
              <tr><td>Id to remove </td><td><input type="text" name="idToRemove" size=8 required></td><td><input type="submit" value="Remove"></td></tr>
          </table>
          <p class="warn">Note that removing a station also ERASES MEASUREMENT DATA related to it.<br>
          Verify with your password below</p>
          <input type="password" name="loginPw" required></td>
        </form>
      </div>
    </div>

  </div>

</div>
