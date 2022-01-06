# Commands

A collection of commands that might be useful for different player OSs

## Volumio

Volumio documentation for creating commands
- [volumio command](https://volumio.github.io/docs/API/Command_Line_Client.html)
- [REST commands](https://volumio.github.io/docs/API/REST_API.html)

### Stop, Play, Pause
```
volumio stop
volumio play
volumio pause
```

### Set volume to a value (15)

```
volumio volume 15
```

Commands to set the volume can be generated with the vols_conf.py script

### Play a radio station from a URL (http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one)
```
curl localhost:3000/api/v1/replaceAndPlay -H "Content-Type: application/json" -d '{"item":{"service":"webradio","uri":"http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one"}}'
```

## Moode

### Stop, Play, Pause
```
mpc -q stop
mpc -q play
mpc -q pause
```

### Set volume to a value (15)

```
mpc volume 15 && sqlite3 /var/local/www/db/moode-sqlite3.db "UPDATE cfg_system SET value='15' WHERE param='volknob'"
```

Commands to set the volume can be generated with the vols_conf.py script

### Play a radio station from a URL (http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one)
```
mpc -q clear && mpc -q add http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one && mpc -q play
```

## MPD

### Stop, Play, Pause
```
mpc -q stop
mpc -q play
mpc -q pause
```

### Set volume to a value (15)
```
mpc -q volume 15
```

Commands to set the volume can be generated with the vols_conf.py script

### Play a radio station from a URL (http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one)
```
mpc -q clear && mpc -q add http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one && mpc -q play
```



