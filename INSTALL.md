# Installation
VISE can be installed in two ways:
 * Docker image :
   * There is no need to install or compile any libraries. All the dependencies come pre-installed and pre-compiled.
   * This method requires running docker (and hence VISE) as root (i.e. Administration) which may not be safe.

 * Source code :
   * This method requires user to compile and install all the dependant libraries (Boost, Imagemagick, cmake, fastann)
   * The compiled VISE binaries can be executed by any user (i.e. non Administrative account) and hence is safer.

For users without prior knowledge of compiling and installing libraries, we recommend the Docker image based method while 
for more advanced users, we advise to compile VISE from source code.

## Docker based installation
### Ubuntu
 1. Install docker : based on instructions at [docker website](https://docs.docker.com/engine/installation/linux/ubuntu/#install-using-the-repository)
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

 2. Download and extract VISE archive from www.robots.ox.ac.uk/~vgg/software/vise/downloads/docker/vise_docker-1.0.0.zip
```
wget www.robots.ox.ac.uk/~vgg/software/vise/downloads/docker/vise_docker-1.0.0.zip
unzip vise_docker-1.0.0.zip
cd vise_docker-1.0.0/
./load.sh
```

### MacOS
 1. Install docker
![Docker Website: to download docker for MacOS](doc/help/docker/docker_website_mac_download.png)
![To install, drag and drop docker file to Applications](doc/help/docker/docker_drop_to_applications.png)
![To install, drag and drop docker file to Applications](doc/help/docker/docker_taskbar_status.png)

 2. Download and extract VISE archive from www.robots.ox.ac.uk/~vgg/software/vise/downloads/docker/vise_docker-1.0.0.zip
![Download and extract the VISE archive: vise_docker-1.0.0.zip](doc/help/docker/extracted_vise_archive.png)

 3. Load VISE docker image and Start/Stop the VISE container to use this application.
![Download and extract the VISE archive: vise_docker-1.0.0.zip](doc/help/docker/extracted_vise_archive.png)
   * Click `macos_load.command` (only the first time) script to load the VISE image container
   * Click `macos_start.command` to start the VISE (or, Image Matcher) tool
   * Click `macos_stop.command` to stop it (do not forget)

## Compiling from source code
### Ubuntu

### MacOS


