<?php
defined('BASEPATH') OR exit('No direct script access allowed');
class Login_model extends CI_Model{

  private function get_password($uname){
    $this->db->select('password');
    $this->db->from('webusers');
    $this->db->where('username', $uname);
    return $this->db->get()->result_array();
  }

  public function verify_password($uname){
    if (!empty($this->Login_model->get_password($uname)[0]['password'])){
      if (password_verify($this->input->post('loginPw'), $this->Login_model->get_password($uname)[0]['password'])) {
        return true;
      }
    }
  }

}
?>
