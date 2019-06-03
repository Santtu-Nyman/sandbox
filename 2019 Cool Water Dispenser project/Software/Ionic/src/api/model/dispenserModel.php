<?php
headers('Access-Control-Allow-Origin' , '*');
headers('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, PUT');
headers('Accept','application/json');
headers('content-type','application/json');
headers('content-type: text/html');

class CwdDatabase{
    
    private $db;

    public function __construct($dbConnection){
        $this->db = $dbConnection;

        if($dbConnection->connect_errno){
            echo "connection failed";
        }
    }

    public function GetData(){

        //Area
        $result = $this->db->query("SELECT * From floor 
        LEFT JOIN dispenser 
        ON dispenser.id = floor.id");
        $dbdata = array();

        while($row = $result->fetch_assoc())
        {
            $dbdata[]=$row;
        }

        return json_encode($dbdata); 

        //building

    }

/*     public function GetBuildingData(){
        $sql = "SELECT buildingName, blueprint, building_id FROM floor";
        $result = $this->db->query($sql);
        $dbdata = array();
        
        if ($result->num_rows > 0) {
            // output data of each row
            while($row = $result->fetch_assoc()) {
                $row["buildingName"];
                $row["blueprint"];
                $row["building_id"]; 
                $dbdata = $result->fetch_all(MYSQLI_ASSOC);
            }
            echo json_encode($dbdata);
        } else {
            http_response_code(404);
        }
    } */
}

?>