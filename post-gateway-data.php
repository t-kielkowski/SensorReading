<?php

$servername = "localhost";
$dbname = "mojmiodp_czujniki-odczyt";
$username = "mojmiodp_czujniki-odczyt";
$password = ".";
$api_key_value = "tPmAT5Ab3j7F9";

if ($_SERVER["REQUEST_METHOD"] == "POST") 
{
    $api_key = test_input($_POST["api_key"]);

    if($api_key == $api_key_value) 
    {
		$GatewayId= test_input($_POST["GatewayId"]);
        $SHT31_Temp = test_input($_POST["SHT31_Temp"]);
		$SHT31_Mois = test_input($_POST["SHT31_Mois"]);
		$BMP280_Temp = test_input($_POST["BMP280_Temp"]);
		$BMP280_Pressure = test_input($_POST["BMP280_Pressure"]);
        $BH1750_Luks = test_input($_POST["BH1750_Luks"]);
		$SoilMoisture = test_input($_POST["SoilMoisture"]);	
		$BatteryLevel= test_input($_POST["BatteryLevel"]);
        
        $conn = new mysqli($servername, $username, $password, $dbname);

        if ($conn->connect_error)
            die("Connection failed: " . $conn->connect_error);        
		
		$sql = "INSERT INTO CentralGateway (GatewayId, SHT31_Temperature, SHT31_Moisture, BMP280_Temperature, BMP280_Pressure, BH1750_Luks, SoilMoisture, BatteryLevel)
        VALUES ('" . $GatewayId . "', '" . $SHT31_Temp . "', '" . $SHT31_Mois . "', '" . $BMP280_Temp . "', '" . $BMP280_Pressure . "', '" . $BH1750_Luks . "', '" . $SoilMoisture . "', '" . $BatteryLevel . "')";    
		
        if ($conn->query($sql) === TRUE)         
            echo "New record created successfully";        
        else 
            echo "Error: " . $sql . "<br>" . $conn->error;        
    
        $conn->close();
    }
    else 
        echo "Wrong API Key provided.";  
}
else 
    echo "No data posted with HTTP POST.";

function test_input($data) 
{
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}
