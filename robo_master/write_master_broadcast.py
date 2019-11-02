import json

class device:
    def __init__(self, id, ip):
        self.ip_address = ip
        self.device_id = id
    def dictionary(self):
        return {"id" : int(self.device_id), "ip" : str(self.ip_address)}
        
def create_master_broadcast_json_text(master_id, device_list):
    json_content = {"master_device_id" : int(master_id), "device_list" : []}
    for device in device_list:
        json_content["device_list"].append(device.dictionary())
    json_text = json.dumps(json_content)
    return json_text
    
device_list = []
device_list.append(device(5, "1.2.3.45"))
device_list.append(device(7, "1.2.3.79"))
device_list.append(device(10, "1.2.3.95"))
device_list.append(device(15, "1.2.3.125"))
json_text = create_master_broadcast_json_text(9, device_list)
file = open("master_broadcast.json", "w")
file.write(json_text)
file.close()
exit()