import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, True)
sock.bind(("0.0.0.0", 1732))

message, master_address = sock.recvfrom(512)
message = bytearray(message)

if len(message) < 16 or str(message[0:15]) != "TVT17SPL_MASTER" :
	exit()
	
master_id = int(message[15])
device_count = (len(message) - 16) / 5
devices = []
for i in range(device_count) :
	device_ip = int(message[16 + (i * 5)]) + (256 * int(message[16 + (i * 5) + 1])) + (65536 * int(message[16 + (i * 5) + 2])) + (16777216 * int(message[16 + (i * 5) + 3]))
	device_id = int(message[16 + (i * 5) + 4])
	devices.append({ "ip" : device_ip, "id" : device_id })

print("Master device " + str(master_id) + " found at address " + str(master_address))
for i in range(device_count) :
	print("Device " + str(devices[i]["id"]) + " found at address " + str((devices[i]["ip"] / 16777216) % 256) + "." + str((devices[i]["ip"] / 65536) % 256) + "." + str((devices[i]["ip"] / 256) % 256) + "." + str(devices[i]["ip"] % 256) + "")
