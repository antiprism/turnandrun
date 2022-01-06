system = "" # no system set

# Choose your system by uncommenting one of the following (i.e. delete
# the leading two characters '# ')
# system = "volumio"
# system = "moode"
# system = "mpd"

# Set your dial parameters
divs = 100         # Number of volume divisions
min_val = 100     # ADS1X15 reading for volume 0
max_val = 26300   # ADS1X15 reading for volume 100
max_volume = 100  # Maximum value for the volume

# Shouldn't need to change anything below

diff = max_val - min_val
for div in range (divs + 1):
    frac = float(div)/divs
    volume = int(max_volume * frac)
    val = min_val + int(frac * diff)
    if system == "volumio":
        print("%05d = volume_%03d,volumio volume %d" %(val, volume, volume))
    elif system == "moode":
        print("%05d = volume_%03d,mpc -q volume %d && " % (val, volume, volume) +
              "sqlite3 /var/local/www/db/moode-sqlite3.db " +
              "\"UPDATE cfg_system SET value='%d' " % (volume) +
              "WHERE param='volknob'\"")
    elif system == "mpd":
        print("%05d = volume_%03d,mpc -q volume %d" % (val, volume, volume))
    else:
        print("No system set, edit the script to set the 'system' variable")
        break
