//index1.h
const char HTML1[] PROGMEM = R"====(
<!DOCTYPE HTML>
<html>

<head>
    <title>BME680 Data Logger III</title>
</head>

<body>
    <h2> BME680 Data Logger III</h2>
    <br> 
    <br> Temperature: %TEMP% F.
    <br> Humidity: %HUM% %%
    <br> Barometric Pressure: %SEALEVEL%  inHg.
    <br> Gas Resistance:  %GAS% KOhms
    <br> 
    <br> Air Quality:  %AIR%
    <br> 
    <br> Client IP: %CLIENTIP%
    <br>
    <br>
    <h2><a href="%LINK%/SdBrowse" >File Browser</a>
    <br>
    <br>
    <a href="%LINK%/Graphs" >Graphed BME680 Observations</a></h2>
    <br>
</body>

</html>
)====";
