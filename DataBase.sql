CREATE TABLE GatewayInformation (
	GatewayId VARCHAR(15) PRIMARY KEY,
	Longitude VARCHAR(10),
	Latitude VARCHAR(10),
    InstallationDate DATE 
);

CREATE TABLE CentralGateway (
    Id INT(6) UNSIGNED AUTO_INCREMENT PRIMARY KEY,
	GatewayId VARCHAR(15),
	SHT31_Temperature VARCHAR(10),    
	SHT31_Moisture VARCHAR(10),
	BMP280_Temperature VARCHAR(10), 
	BMP280_Pressure VARCHAR(10),
	BH1750_Luks VARCHAR(10),
	SoilMoisture VARCHAR(10),
	BatteryLevel VARCHAR(6),	
    ReadingTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	FOREIGN KEY (GatewayId) REFERENCES GatewayInformation(GatewayId)
);

CREATE TABLE WeightReadings (
    Id INT(6) UNSIGNED AUTO_INCREMENT PRIMARY KEY,
	WeightId VARCHAR(10),
	Weight VARCHAR(10),
	Temperature VARCHAR(10), 
	Moisture VARCHAR(10),
	BatteryLevel VARCHAR(6),	
    ReadingTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
