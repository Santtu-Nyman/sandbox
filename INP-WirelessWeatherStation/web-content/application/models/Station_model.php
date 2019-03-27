<?php

class Station_model extends CI_Model{

  public function get_all_stations(){
    $this->db->select('*');
    $this->db->from('stations');
    return $this->db->get()->result_array();
  }

  public function get_station_count(){
    $this->db->from("stations");
    return $this->db->count_all_results();
  }
  public function get_measurement_count(){
    $this->db->select_max('idMeasurement');
    $this->db->from("measurements");
    return $this->db->get()->row()->idMeasurement;
  }

  public function get_recent_measurements(){
    $this->db->select('designation, region, timestamp, temperature, humidity, pressure, illuminance, cmonoxide');
    $this->db->join('stations', 'measurements.idStation = stations.idStation');
    $this->db->from('measurements');
    $this->db->order_by("idMeasurement", "desc");
    $this->db->limit(10);
    return $this->db->get()->result_array();
  }
  public function search_stations_from_database($queryParams){
    $this->db->select('*');
    $this->db->from('stations');
    foreach ($queryParams as $k => $v){
      if (!empty($v)){
        $this->db->like($k, $v);
      }
    }
    //$this->db->get()->result_array(); //debug
    //echo $this->db->last_query(); //debug
    return $this->db->get()->result_array();
  }

  public function get_measurement_by_station_id($queryParams){
    $this->db->select('*');
    $this->db->from('measurements');
    $this->db->where('idStation', $queryParams['id']);
    if (!empty($queryParams['start']) && !empty($queryParams['end'])){
      $this->db->where('timestamp >=', $queryParams['start']);
      $this->db->where('timestamp <=', $queryParams['end']);
    } else {
      $then = mdate('%Y-%m-%d', now()-(7*86400));
      $now = mdate('%Y-%m-%d', now());
      $this->db->where('timestamp >=', $then);
      $this->db->where('timestamp <=', $now);
    }
    return $this->db->get()->result_array();
  }
}
?>
