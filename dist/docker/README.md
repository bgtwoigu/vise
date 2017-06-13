# Docker image of VGG Image Search Engine (VISE)

Assuming that docker has already been installed on a user's computer, the aim 
of this docker image is to provide a one click installation of the VGG Image 
Search Engine (VISE).

## Install Docker
Reference: [docker-installation-guide](https://docs.docker.com/engine/installation/linux/ubuntu/#install-using-the-repository)

```
sudo apt-get remove docker docker-engine
sudo apt autoremove
sudo apt-get install     apt-transport-https     ca-certificates     curl     software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo apt-key fingerprint 0EBFCD88
sudo add-apt-repository    "deb [arch=amd64] https://download.docker.com/linux/ubuntu (lsb_release -cs) stable"
sudo apt-get update
apt-cache madison docker-ce
sudo apt-get install docker-ce=17.03.0~ce-0~ubuntu-xenial
```

## Building VISE image
 * "Dockerfile will define what goes on in the environment inside your container". 
 * continue with [this tutorial](https://docs.docker.com/get-started/part2/#recap-and-cheat-sheet-optional)

```
cd /home/tlm/dev/vise/dist/docker
ls
  Dockerfile  README.md

sudo time -v docker build --no-cache=true -t vise:1.0.0 .  # build the VISE image
sudo docker images -a
sudo docker run vise
sudo docker run -d vise                      # run VISE image in background
sudo docker attach vise

sudo docker ps -a                            # list all running container
sudo docker rm 844694571ca0

sudo docker run -it vise bash                # run bash shell in interactive mode
sudo docker run -p 8080:8080 -it vise bash
sudo time -v docker run --entrypoint -p 8080:8080 -it vise bash -v /home/tlm:/data/images # mounting fs
sudo docker run --rm --entrypoint "" -p 8080:8080 -it vise:1.0.0-beta bash
sudo docker run --rm -p 8080:8080 -it vise:1.0.0-beta
sudo docker run --rm -p 8080:8080 -v ~/:/home/$USER -it vise:1.0.0-beta

sudo docker run --env USER=$USER --rm -p 9971:9971 -p 9973:9973 -v ~/:/home/$USER -it vise:1.0.0-beta
sudo docker run --env USER=$USER --rm -p 9971:9971 -p 9973:9973 -v ~/:/home/$USER -it registry.gitlab.com/vgg/vise/image:latest
sudo docker run --env USER=$USER --rm -p 9971:9971 -p 9973:9973 -v ~/:/home/$USER -it vise:1.0.0

sudo docker run --env USER=$USER --rm -p 8080:8080 -v ~/:/home/$USER -d vise:1.0.0-beta

sudo docker run --rm --entrypoint "" -p 8080:8080 -v ~/:/home/$USER -it vise:1.0.0-beta bash
sudo docker run --rm --entrypoint "" -p 9971:9971 -v ~/:/home/$USER -it vise:1.0.0 bash

sudo docker run --rm --entrypoint "" -p 8080:8080 -v ~/vgg/:/opt/vgg/ -it vise:1.0.0-beta bash

sudo docker stop `sudo docker ps -a -q`
sudo docker rm `sudo docker ps -a -q`
sudo docker rmi `sudo docker images -a -q` -f

sudo docker volume ls
sudo docker create -v ~/ox/vgg --name vgg-shared-store vise:1.0.0-beta /bin/true
sudo docker volume rm $(sudo docker volume ls -f dangling=true -q)

sudo docker save --output vise-1.0.0-beta.tar vise:1.0.0-beta
sudo docker save --output vise-1.0.0.tar registry.gitlab.com/vgg/vise/image:latest
```

## Publishing image to gitlab
```
sudo docker login --username thelinuxmaniac registry.gitlab.com
sudo docker build -t registry.gitlab.com/vgg/vise:1.0.0-beta .
sudo docker push registry.gitlab.com/vgg/vise
```

## Fetching image from gitlab
```
sudo docker login --username thelinuxmaniac registry.gitlab.com
sudo docker pull registry.gitlab.com/vgg/vise/image:latest
```

Abhishek Dutta  
05 June 2017
