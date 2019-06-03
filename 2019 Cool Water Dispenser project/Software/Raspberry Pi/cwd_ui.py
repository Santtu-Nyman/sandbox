import sys
import RPi.GPIO as GPIO
import time
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
sr_data = [ False, False, False, False, False, False, True, True, True, False, False, False ]

#TODO 60ms delay between sensor measurements/pulse

#Proximity sensor pins
TRIG0 = 2
TRIG1 = 17
TRIG2 = 10

ECHO0 = 3
ECHO1 = 27
ECHO2 = 9

DEVICE0 = 0
DEVICE1 = 1
DEVICE2 = 2

lpf_distance0 = 100
lpf_distance1 = 100
lpf_distance2 = 100
lpf_skip_count = [ 0, 0, 0 ]
#en min* jaksa n*it* saatanan taulujen ulkona olevia perkeleen objekteja

#MOTOR pins
MOTOR_A = 26
MOTOR_B = 19
#INSERT_DETECTION = 13
MOTOR_ENABLE = 6
#POWER_ENABLE = 5

#LED pins
sr_clk_pin = 24
sr_in_pin = 23
red_led = 0
green_led = 1
blue_led = 2

#LEDs
GPIO.setup(sr_clk_pin, GPIO.OUT)
GPIO.setup(sr_in_pin, GPIO.OUT)

#Insert detection
#GPIO.setup(INSERT_DETECTION,GPIO.OUT)

#Proximity sensor
GPIO.setup(TRIG0,GPIO.OUT)
GPIO.setup(TRIG1,GPIO.OUT)
GPIO.setup(TRIG2,GPIO.OUT)

GPIO.setup(ECHO0,GPIO.IN)
GPIO.setup(ECHO1,GPIO.IN)
GPIO.setup(ECHO2,GPIO.IN)

#Main power
#GPIO.setup(POWER_ENABLE,GPIO.OUT)
#GPIO.output(POWER_ENABLE, True)

#Motor
GPIO.setup(MOTOR_ENABLE,GPIO.OUT)
GPIO.setup(MOTOR_A,GPIO.OUT)
GPIO.setup(MOTOR_B,GPIO.OUT)
GPIO.output(MOTOR_ENABLE, False)
GPIO.output(MOTOR_A, False)
GPIO.output(MOTOR_B, False)

def my_sleep(t):
    end = time.clock() + t
    while time.clock() < end :
        continue

def set_led(led, color, on) :
    global sr_clk_pin
    global sr_in_pin
    global red_led
    global green_led
    global blue_led
    if led == 2 :
        if on == 0 :
                on = 1
        else :
                on = 0
    led_color_pin_lookup = [ 0, 2, 1, 0, 1, 2, 0, 1, 2 ]
    color = led_color_pin_lookup[3 * led + color]
    previous_led_voltage = sr_data[3 * led + color];
    next_led_voltage = on == 1
    if previous_led_voltage != next_led_voltage :
        sr_data[3 * led + color] = next_led_voltage;
        GPIO.output(sr_clk_pin, False)
        my_sleep(0.000001)
        for i in range(0, 12) :
            GPIO.output(sr_in_pin, sr_data[11 - i])
            my_sleep(0.000001)
            GPIO.output(sr_clk_pin, True)
            my_sleep(0.000001)
            GPIO.output(sr_clk_pin, False)
            
            GPIO.output(sr_clk_pin, False)
            my_sleep(0.000001)
for i in range(0, 12) :
    GPIO.output(sr_in_pin, sr_data[11 - i])
    my_sleep(0.000001)
    GPIO.output(sr_clk_pin, True)
    my_sleep(0.000001)
    GPIO.output(sr_clk_pin, False)



def detect_connector():
    while GPIO.input(INSERT_DETECTION)==0 :
        print("no connector attached")

def trig_pulse(trigger):
    GPIO.output(trigger, False)
    my_sleep(.001)
    GPIO.output(trigger, True)
    my_sleep(0.00001)
    GPIO.output(trigger, False)


def measure(device):
    global lpf_distance0
    global lpf_distance1
    global lpf_distance2
    global lpf_skip_count

    if device==DEVICE0:
        trigger = TRIG0
        echo = ECHO0
        lpf_distance = lpf_distance0

    elif device==DEVICE1:
        trigger = TRIG1
        echo = ECHO1
        lpf_distance = lpf_distance1

    elif device==DEVICE2:
        trigger = TRIG2
        echo = ECHO2
        lpf_distance = lpf_distance2

    lpf_constant = 0.91
    pulse_start = 0
    pulse_end = 0
    
    trig_pulse(trigger)
    echo_timeout = time.clock() + .06
    
    while echo_timeout > time.clock() and GPIO.input(echo) == 0 :
        continue
    pulse_start = time.clock()
    while echo_timeout > time.clock() and GPIO.input(echo) == 1 :
        continue
    pulse_end = time.clock()
    
    good_sample = pulse_end < echo_timeout
    
    #if echo_timeout > pulse_end :
    #    my_sleep(echo_timeout - pulse_end)

    pulse_duration = pulse_end - pulse_start    #Get pulse duration for proximity sensor
    distance = pulse_duration * 17150   #Muliply pulse to obtain distance
    distance = round(distance, 2)       #Round distance to two decimal

    #return distance;

    #print("Device:", device, "Distance:", distance,"cm")

    if distance > 100 or distance < 0:
        distance = 100

    lpf_distance = lpf_constant * lpf_distance + (1 - lpf_constant) * distance

    if good_sample or lpf_skip_count[device] > 4 :
        if device==DEVICE0:
            lpf_distance0 = lpf_distance

        elif device==DEVICE1:
            lpf_distance1 = lpf_distance

        elif device==DEVICE2:
            lpf_distance2 = lpf_distance
        lpf_skip_count[device] = 0
    else :
        lpf_skip_count[device] = lpf_skip_count[device] + 1

    return lpf_distance

def set_green(device):
    set_led(device, green_led, 1)
    set_led(device, red_led, 0)
    set_led(device, blue_led, 0)

def set_blue(device):
    set_led(device, green_led, 0)
    set_led(device, red_led, 0)
    set_led(device, blue_led, 1)
    
def set_red(device):
    set_led(device, green_led, 0)
    set_led(device, red_led, 1)
    set_led(device, blue_led, 0)

def set_none(device):
    set_led(device, green_led, 0)
    set_led(device, red_led, 0)
    set_led(device, blue_led, 0)

def print_order(device):
    if device==DEVICE0:
        print("!1")
	sys.stdout.flush()

    if device==DEVICE1:
        print("!2")
	sys.stdout.flush()

    if device==DEVICE2:
        print("!3")
	sys.stdout.flush()
        
def portioning_indicator(device, current_change_duration, select_duration, pouring_duration, tube_withdrawal_duration):
    start = time.clock()
    is_selected = False

    while(measure(device) < 20 and not is_selected):
        set_green(device)
        if(time.clock() - start >= select_duration):
            is_selected = True

    if(is_selected):
        print_order(device)
        set_blue(device)        
        
        GPIO.output(MOTOR_ENABLE, False)
        my_sleep(current_change_duration) #CURRENT DIRECTION CHANGE REQUIREMENT
        GPIO.output(MOTOR_ENABLE, True)
        GPIO.output(MOTOR_A, False)
        GPIO.output(MOTOR_B, True)
        my_sleep(pouring_duration + tube_withdrawal_duration)
        
        set_red(device)
        
        GPIO.output(MOTOR_ENABLE, False)
        my_sleep(current_change_duration) #CURRENT DIRECTION CHANGE REQUIREMENT
        GPIO.output(MOTOR_ENABLE, True)
        GPIO.output(MOTOR_B, False)
        GPIO.output(MOTOR_A, True)
        my_sleep(tube_withdrawal_duration) #TODO MEASURE THE EXACT TIME NEEDED FOR WATER TO BE PULLED BACK IN THE TANK (THE WATER IN THE TUBE ONLY)
        
        set_none(device)
        GPIO.output(MOTOR_A, False)
        GPIO.output(MOTOR_B, False)
        GPIO.output(MOTOR_ENABLE, False)
    else :
        set_none(device)
                    

while True:
    #detect_connector()
    #measure()
    #portioning_indicator(led, acception_indication_period, current_change_duration, pouring_duration, tube_withdrawal_duration
    #note: pouring takes around 5 seconds just to get the water out of the tubings
    
    
    portioning_indicator(DEVICE0, 0.001, 2.5, 10, 5)
    portioning_indicator(DEVICE1, 0.001, 2.5, 20, 5)
    portioning_indicator(DEVICE2, 0.001, 2.5, 30, 5)

    
                    
#DEBUG
    #print("meh ",distance," ",lpf_distance)
