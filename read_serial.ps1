$port = New-Object System.IO.Ports.SerialPort COM10,115200,None,8,one
$port.Open()
Start-Sleep -Seconds 3
$s = $port.ReadExisting()
$port.Close()
Write-Host $s
