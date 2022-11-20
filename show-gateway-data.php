<!DOCTYPE html>
<html>

<body>
    <a href="index.html">Home</a>
    <?php

    $servername = "localhost";
    $dbname = "mojmiodp_czujniki-odczyt";
    $username = "mojmiodp_czujniki-odczyt";
    $password = ".";

    $conn = new mysqli($servername, $username, $password, $dbname);

    if ($conn->connect_error)
        die("Connection failed: " . $conn->connect_error);  

    $sql = "SELECT SHT31_Temperature, SHT31_Moisture, BMP280_Temperature, BMP280_Pressure, BH1750_Luks, SoilMoisture, BatteryLevel, ReadingTime FROM CentralGateway ORDER BY id DESC";

    echo
    '<table cellspacing="5" cellpadding="5">
      <tr> 
      <td>SHT31_Temp</td> 
      <td>SHT31_Mois</td> 
      <td>BMP280_Temp</td> 
      <td>BMP280_Press</td> 
      <td>BH1750_Luks</td> 
      <td>SoilMoisture</td> 
      <td>BatteryLevel</td> 
      <td>ReadingTime</td> 
      </tr>';

    if ($result = $conn->query($sql)) 
    {
        while ($row = $result->fetch_assoc()) 
        {
            echo '<tr>             
                <td>' . $row["SHT31_Temperature"] . ' °C</td> 
                <td>' . $row["SHT31_Moisture"] . ' %</td>
                <td>' . $row["BMP280_Temperature"] . ' °C</td>   
                <td>' . $row["BMP280_Pressure"] . ' hPa</td> 
                <td>' . $row["BH1750_Luks"] . ' lux</td> 
				<td>' . $row["SoilMoisture"] . ' %</td> 
				<td>' . $row["BatteryLevel"] . ' %</td> 
				<td>' . $row["ReadingTime"] . '</td> 
              </tr>';
        }
        $result->free();
    }

    $conn->close();
    ?>
    </table>
</body>
</html>