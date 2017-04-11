#The docker images 

##GCC
This image will be used to build the client and server tools.

```
docker build -t tools/gcc "https://github.com/marc-despland/apache-bench.git#master:/docker/gcc"
```

##Apache
The apache instance use for the test. It is configured as a reverse proxy

```
docker build -t tools/rp "https://github.com/marc-despland/apache-bench.git#master:/docker/reverse-proxy"
```

##Server


##Client


#Building the tools

First clone this repository

```
git clone https://github.com/marc-despland/apache-bench.git
```

Then start the GCC to build the code
```
docker run -it --rm -v /home/mde/Projects/apache-bench/tools:/project:Z tools/gcc make
```