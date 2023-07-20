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