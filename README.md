# parallel-plotter
The Parallel poc2+ gpu plotter

# How to use
```shell
Options:
  --version             show program's version number and exit
  -h, --help            show this help message and exit
  -V, --verbose         verbose, default: %default
  -t, --test            test mode, default: %default
  -i ID, --id=ID        plot id, default: 0
  -s SN, --sn=SN        start nonce, default: 0
  -n NUM, --num=NUM     number of nonces, default: 10000
  -w WEIGHT, --weight=WEIGHT
                        plot file weight, default: 1 (GB)
  -m MEM, --mem=MEM     memory to use, default: 65535 (GB)
  -p, --plot            run plots generation, default: %default
  --step=STEP           hash calc batch, default: 8192
  --gws=GWS             global work size, default: 0
  --lws=LWS             local work size, default: 0
  -l LEVEL, --level=LEVEL
                        log level, default: info
                        
Example:
  ./parallel-plotter.exe --plot -i 1234567890 -s 0 -n 100000 D:\ E:\ F:\ I:\ J:\ K:\
```
# Pre-required
* boost-compute
* opencl

# Build
```shell
git clone https://github.com/HarvestStars/poc-parallel-plotter.git --recursive
cd parallel-plotter && mkdir build
# unix like
cmake ..
# windows
cmake .. -A x64 "-DVCPKG_TARGET_TRIPLET=x64-windows" "-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```
