<?php
defined('BASEPATH') OR exit('No direct script access allowed');

class Error extends CI_Controller {

  public function no_login(){
    $data['selected'] = "";
    $data['view']='errors/no_login';
    $this->load->view('layout/content', $data);
  }
}
?>
