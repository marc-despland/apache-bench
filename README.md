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
The build version of the server use to delay the HTTP response

```
docker build -t tools/server "https://github.com/marc-despland/apache-bench.git#master:/docker/server"
```

##Client
The build version of the client use to send http request at a given rate

```
docker build -t tools/client "https://github.com/marc-despland/apache-bench.git#master:/docker/client"
```


#Building the tools

First clone this repository

```
git clone https://github.com/marc-despland/apache-bench.git
```

Then start the GCC to build the code
```
docker run -it --rm -v /home/mde/Projects/apache-bench/tools:/project:Z tools/gcc make
```

And copy client and server in the right docker folder and rebuilds images, or start them using the GCC image