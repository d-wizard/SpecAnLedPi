import os
import time

THIS_SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))

os.chdir(THIS_SCRIPT_PATH)

time.sleep(2)

while not os.path.exists('kill'):
   os.system('sudo ./AmbientDisplay 0')
   time.sleep(2)
