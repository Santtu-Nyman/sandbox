<div id="topnav" class="container">
  <h2>INP-Weather stations</h2>
  <?php
  if(!isset($_SESSION['loggedIn'])){
    $_SESSION['loggedIn'] = false; //TODO find a proper place to initialize this
  }

  if($_SESSION['loggedIn']==false){
    echo  '<form action="'.site_url('login/siteLogin').'" method="post">
          '.$this->session->flashdata('error').'
          <input type="text" placeholder="User ID" name="loginID" size="12" style="margin: 8px 0px;">
          <input type="password" placeholder="Password" name="loginPw" size="17" style="margin: 8px 0px;">
          <button id="loginbtn" type="submit" value="Login"><span>Login</span></button>
          </form>';
  }else{
    echo '<form action="'.site_url('login/siteLogout').'" method="post">
          <p id=userid>'.$_SESSION['user'].'</p>
          <button id="loginbtn" type="submit" value="Logout"><span>Logout</span></button>
          </form>';
  }
  ?>
</div>
