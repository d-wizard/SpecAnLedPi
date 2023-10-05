# SpecAnLedPi

## Things to Install

```
sudo apt-get install cmake
sudo apt-get install scons
sudo apt-get install libasound2-dev
```

## Build Instuctions

```
git submodule update --init --recursive
cd modules/
./build_modules.sh
cd ../
scons
```

## Microphone Setup Notes
The microphone name is specified in "settings.json" in the "microphone_name" field.

To determine the card #, run the following command:
```
arecord -l
```

The microphone name is "hw:#" where # is the card number.


