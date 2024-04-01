<?php

class Management_model extends CI_Model{

  public function add_station_to_database($stationInfo){
    $this->db->set($stationInfo);
    $this->db->insert('stations');
  }
  public function remove_station_from_database($stationId){
    $this->db->where('idStation', $stationId);
    $this->db->delete('stations');
  }
  public function modify_station_in_database($stationId, $data){
    $this->db->where('idStation', $stationId);
    $this->db->update('stations', $data);
  }
}
?>
