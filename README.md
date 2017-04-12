# The docker images 

## GCC
This image will be used to build the client and server tools.

```
docker build -t tools/gcc "https://github.com/marc-despland/apache-bench.git#master:/docker/gcc"
```

## Apache
The apache instance use for the test. It is configured as a reverse proxy

```
docker build -t tools/rp "https://github.com/marc-despland/apache-bench.git#master:/docker/reverse-proxy"
```

## Server
The build version of the server use to delay the HTTP response

```
docker build -t tools/server "https://github.com/marc-despland/apache-bench.git#master:/docker/server"
```

## Client
The build version of the client use to send http request at a given rate

```
docker build -t tools/client "https://github.com/marc-despland/apache-bench.git#master:/docker/client"
```


# Building the tools

First clone this repository

```
git clone https://github.com/marc-despland/apache-bench.git
```

Then start the GCC to build the code
```
docker run -it --rm -v /home/mde/Projects/apache-bench/tools:/project:Z tools/gcc make
```

And copy client and server in the right docker folder and rebuilds images, or start them using the GCC image

# Running tests

## Start delay server
This command will run a server that will listening on port 8080 and take 10s to send the answer
```
docker run -d --name server10 tools/server -p 8080 -s 10
```

To find the container ip
```
docker inspect --format '{{ .NetworkSettings.IPAddress}}' server10
```

To test the server with curl with the right IP address
```
curl http://172.17.0.2:8080
```

## Start the reverse-proxy

```
docker run -it --rm --name rp --link server10:front tools/rp
```

It will start with a shell. There are 2 commands useful : 
* choose_mpm.sh : to switch the worker module
* start_apache.sh : to start apache in foreground

If you need to exec another shell on the container:
```
docker exec -it rp /bin/bash
```

## Start the client

```
docker run -it --rm --link rp:target tools/client -t target -c 80 -n 100 -w 10
```

* n : The number of requests to send
* w : The rate (request per second) to send them

