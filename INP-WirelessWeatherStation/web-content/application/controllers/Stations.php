<?php
defined('BASEPATH') OR exit('No direct script access allowed');

class Stations extends CI_Controller {
  public function index(){
    $this->stations_summary();
  }

  public function stations_summary(){
    $this->load->model('Station_model');
    $data['selected']          = "stations_summary";
    $data['view']              = 'stations/summary';
    $data['measurements']      = $this->Station_model->get_recent_measurements();
    $data['station_count']     = $this->Station_model->get_station_count();
    $data['measurement_count'] = $this->Station_model->get_measurement_count();
		$this->load->view('layout/content', $data);
	}
  public function show_all_stations(){
    $this->load->model('Station_model');
    $data['selected']   = "show_all_stations";
    $data['view']       = 'stations/all_stations';
    $data['a_stations'] = $this->Station_model->get_all_stations();
		$this->load->view('layout/content', $data);
	}

  public function station_search(){
    $this->load->model('Station_model');
    $data['selected'] = "station_search";
    $data['view']     = "stations/station_search";
    if (isset($this->input->post()['data'])){
        print_r($this->input->post()['data']);
        $queryParams=$this->input->post()['data'];
        $data['results'] = $this->Station_model->search_stations_from_database($queryParams);
        //print_r($data['results']);
    } else {
       $data['results'] = [];
    }
    $this->load->view('layout/content', $data);
	}

  public function data_visualization(){
    $data['selected']="visualization";
    $data['view']='stations/visualization';
		$this->load->view('layout/content', $data);
  }
  public function ajaxtest(){
    $this->load->model('Station_model');
    $queryParams['id']    =$this->input->post('id');
    $queryParams['start'] =$this->input->post('start');
    $queryParams['end']   =$this->input->post('end');
    $data = $this->Station_model->get_measurement_by_station_id($queryParams);
    print_r(json_encode($data, true));
  }
}
?>
