<?php
defined('BASEPATH') OR exit('No direct script access allowed');

class Management extends CI_Controller {
  function __construct(){
    parent::__construct();
    if($_SESSION['loggedIn']== false){
      $data['selected']="station_management";
      redirect('Error/no_login');
    }
  }

  public function station_management(){
    $data['selected']="station_management";
    $data['view']='stations/manage_stations';
    $this->load->view('layout/content', $data);
  }

  public function station_add(){
    $this->form_validation->set_rules('latitude', 'Latitude', 'numeric');
    $this->form_validation->set_rules('longitude', 'Longitude', 'numeric');
    $this->form_validation->set_rules('interval', 'Measurement interval', 'integer');
    if ($this->form_validation->run() == TRUE){
      $this->load->model('Management_model');
      $dataToAdd=$this->input->post();
      $this->Management_model->add_station_to_database($dataToAdd);
      redirect("Stations/show_all_stations");
    }
    $data['selected']="station_management";
    $data['view']='stations/manage_stations';
    $this->load->view('layout/content', $data);
  }

  public function station_modify(){
    $this->form_validation->set_rules('id', 'Station ID', 'required|integer');
    $this->form_validation->set_rules('latitude', 'Latitude', 'numeric');
    $this->form_validation->set_rules('longitude', 'Longitude', 'numeric');
    $this->form_validation->set_rules('interval', 'Measurement interval', 'integer');
    if ($this->form_validation->run() == TRUE){
      $this->load->model('Management_model');
      $id = $this->input->post("id");
      foreach (array_slice($this->input->post(), 1) as $key => $value) {
        if (!empty($value)){
          $newData[$key] = $value;
        }
      }
      if (!empty($newData)){
        $this->Management_model->modify_station_in_database($id, $newData);
        redirect("Stations/show_all_stations");
      } else {
        $this->session->set_flashdata('error', 'Nothing to change...');
        redirect($_SERVER['HTTP_REFERER']);
      }
    }
    $data['selected']="station_management";
    $data['view']='stations/manage_stations';
    $this->load->view('layout/content', $data);
  }

  public function station_remove(){
    $this->load->model('Login_model');
    if ($this->Login_model->verify_password($_SESSION['user'])) {
      $this->load->model('Management_model');
      $id = $this->input->post('id');
      $this->Management_model->remove_station_from_database($id);
      redirect("Stations/show_all_stations");
    } else {
      $this->session->set_flashdata('error', 'Invalid password');
      redirect($_SERVER['HTTP_REFERER']);
      //echo "<p>NO CHANCE</br>TRY USING A CORRECT PASSWORD NEXT TIME</br></br>I'M YOUR FRIENDLY HELP MESSAGE. HAVE A NICE DAY</p>";
    }
  }
}
?>
