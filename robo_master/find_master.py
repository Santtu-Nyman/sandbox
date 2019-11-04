import socket

def find_master():
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, True)
	sock.bind(("0.0.0.0", 1732))

	message, master_address = sock.recvfrom(512)
	message = bytearray(message)
	if len(message) < 16 or str(message[0:15]) != "TVT17SPL_MASTER"  :
		return { "master_device_id" : -1, "master_device_address" : "0.0.0.0", "device_list" : [] }
		
	device_count = (len(message) - 16) / 5
	configuration = { "master_device_id" : int(message[15]), "master_device_address" : str(master_address[0]), "device_list" : [] }
	for i in range(device_count) :
		device_ip = str(int(message[16 + (i * 5) + 3])) + "." + str(int(message[16 + (i * 5) + 2])) + "." + str(int(message[16 + (i * 5) + 1])) + "." + str(int(message[16 + (i * 5)]))
		device_id = int(message[16 + (i * 5) + 4])
		configuration["device_list"].append({ "ip" : device_ip, "id" : device_id })
	return configuration

configuration = find_master()
print("Master device " + str(configuration["master_device_id"]) + " found at address " + configuration["master_device_address"])
for i in range(len(configuration["device_list"])) :
	print("Device " + str(configuration["device_list"][i]["id"]) + " found at address " + configuration["device_list"][i]["ip"] + "")
exit()
