<?php
/*header('content-type: text/html');
header("access-control-allow-origin: *");


class DataBuilding{
    private $db;

    public function __construct($dbConnection){
        $this->db = $dbConnection;

        if($dbConnection->connect_errno){
            echo "connection failed";
        }
    }
    
    public function GetBuilding(){
        $sql = "SELECT buildingName, blueprint, building_id FROM floor";
        $result = $this->db->query($sql);
        $dbdata = array();
        
        if ($result->num_rows > 0) {
            // output data of each row
            while($row = $result->fetch_assoc()) {
                $row["buildingName"];
                $row["blueprint"];
                $row["building_id"];/* 
                $dbdata = $result->fetch_all(MYSQLI_ASSOC); */
            }
            echo json_encode($dbdata);
        } else {
            http_response_code(404);
        }
    }
}*/

?>