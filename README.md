# Mini iPerf
A simple throughput measurement tool

## Usage
1. Create object file
```sh
make all
```
2. Run Server
```sh
./miniIPerf -s -p 8080
```
where `-p` is an integer within [1024, 65535]

3. Run Client
```sh
./miniIPerf -c -h 127.0.0.1 -p 8080 -t 20
```
where `-h` is server address, `-p` is an integer within [1024, 65535] and `-t` is the measurement duration in unit of second. 