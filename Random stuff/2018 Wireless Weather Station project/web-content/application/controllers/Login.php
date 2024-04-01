<?php
defined('BASEPATH') OR exit('No direct script access allowed');
class Login extends CI_Controller{

  public function siteLogin(){
    $this->load->model('Login_model');
    if ($this->Login_model->verify_password($this->input->post('loginID'))){
      $_SESSION['user']=$this->input->post('loginID');
      $_SESSION['loggedIn']=true;
      redirect('Management/station_management');
    }
    $this->session->set_flashdata('error', 'Login failed');
    redirect($_SERVER['HTTP_REFERER']);
  }

  public function siteLogout(){
    $_SESSION['loggedIn']=false;
    redirect($_SERVER['HTTP_REFERER']);
  }
}
?>
