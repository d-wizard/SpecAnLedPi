# SpecAnLedPi

```
sudo apt-get install cmake
sudo apt-get install scons
sudo apt-get install libasound2-dev
```

## Build Instuctions

```
git submodule update --init --recursivecd modules
cd modules/
./build_modules.sh
cd ../
scons
```