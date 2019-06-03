<?php
defined('BASEPATH') OR exit('No direct script access allowed');

class Measurement extends CI_Controller {

  public function add_new_measurement(){
    $this->load->model('Measurement_model');
    //print_r($_POST);
    $sender_hmac = hash_hmac('sha256', '52-5951024400', '2placeholder');
    $TESTPOST = array(
      "signature" => $sender_hmac,
      "idStation" => '5',
      "messageIndex" => '2',
      "temperature" => '-5',
      "humidity" => '95',
      "pressure" => '1024',
      "illuminance" => '400'
    );
    if($this->Measurement_model->add_measurement_to_database($_POST)){
      echo "ACK";
    }else{
      echo "NACK";
    }
  }
}
?>
