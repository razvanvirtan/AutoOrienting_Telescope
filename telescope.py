import serial
import datetime
from simple_term_menu import TerminalMenu
import geocoder

DEVICE = '/dev/rfcomm0'
BAUD_RATE = 9600

menu_options = ["47 TUCANAE, CLST", "BEEHIVE CLUSTER", "HYADES CLUSTER",
                "JEWEL BOX, CLSTR", "M4 CLUSTER", "M12 CLUSTER",
                "M68 CLUSTER", "CONE NEBULA", "CRESCENT NEBULA",
                "DUMBBELL NEBULA", "EAGLE NEBULA", "ESKIMO NEBULA",
                "HELIX NEBULA", "HOURGLASS NEBULA", "LAGOON NEBULA",
                "ORION NEBULA", "OWL NEBULA", "SATURN NEBULA"]

coordinates = [{'right_ascension':6.02363, 'declination':-72.08128},
               {'right_ascension':130.1, 'declination':19.98333},
               {'right_ascension':66.75, 'declination':15.86667},
               {'right_ascension':193.41254, 'declination':-60.36161},
               {'right_ascension':245.89775, 'declination':-26.52536},
               {'right_ascension':251.81042, 'declination':-1.94781},
               {'right_ascension':189.86675, 'declination':-26.74281},
               {'right_ascension':100.24271, 'declination':9.89558},
               {'right_ascension':303.02700, 'declination':38.35497},
               {'right_ascension':299.90133, 'declination':22.72150 },
               {'right_ascension':274.70021, 'declination':-13.80694},
               {'right_ascension':112.29492, 'declination':20.91175},
               {'right_ascension':337.41025, 'declination':-20.83686},
               {'right_ascension':204.89583, 'declination':-67.38083},
               {'right_ascension':270.92504, 'declination':-24.38},
               {'right_ascension':83.82163, 'declination':-5.39081},
               {'right_ascension':168.69875, 'declination':55.01914},
               {'right_ascension':316.04513, 'declination':-11.36342}
]

# Connect to the device
s = serial.Serial(DEVICE, BAUD_RATE)

# Get and send time, date, location and object celestial coordinates
menu = TerminalMenu(menu_options, title = "Choose your celestial body: ")
menu_entry_index = menu.show()
print("Sending to telescope...")
print(menu_options[menu_entry_index], coordinates[menu_entry_index]['right_ascension'], coordinates[menu_entry_index]['declination'])

g = geocoder.ip('me')
print(g.latlng)

s.write(bytes(str(coordinates[menu_entry_index]['right_ascension']), 'utf-8'))
s.write(b'\n')
s.write(bytes(str(coordinates[menu_entry_index]['declination']), 'utf-8'))
s.write(b'\n')
s.write(bytes(str(g.latlng[0]), 'utf-8'))
s.write(b'\n')
s.write(bytes(str(g.latlng[1]), 'utf-8'))
s.write(b'\n')
e = datetime.datetime.utcnow()
s.write(bytes(str(e.year), 'utf-8'))
s.write(b'\n')
s.write(bytes(str(e.month), 'utf-8'))
s.write(b'\n')
s.write(bytes(str(e.day), 'utf-8'))
s.write(b'\n')
s.write(bytes(str(e.hour), 'utf-8'))
s.write(b'\n')
s.write(bytes(str(e.minute), 'utf-8'))
s.write(b'\n')


