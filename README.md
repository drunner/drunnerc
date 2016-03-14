## dRunner

# Status

In active development, generally working. Supports both version 1 and version 2 dServices.

# Overview

dRunner is a stand-alone executable and a set of conventions to make it easy to make, install,
configure and use compatible Docker containers ("dServices") on a Debian host via the 
command line interface.

dRunner eliminates the need to manually store and manage scripts to use the Docker containers,
or type in long docker run commands. It also provides a configurable way (per host) to bring up 
services that consist of multiple Docker containers.

Features:
* dRunner compatible Docker Images are self contained - everything dRunner needs is inside
* Simple discoverable commands for using compatible services (no manual needed)
* Flexible configuration for each service, stored in Docker Volume containers that are managed for you
* Services can consist of any number of containers
* Backup an entire service to a single file, trivially restore on another machine
* Destroying a service leaves the machine in a clean state, no cruft left
* Everything in containers is run as a non-root user, drunner on host runs as non-root
* Trivial to install a service multiple times with different configurations (e.g. mulitple minecraft servers)
* Ansible friendly for automation (see [Exit Codes](https://github.com/j842/dr#exit-codes) below).
* Small footprint: everything dRunner creates on the host is contained in one folder of your choice (apart from Docker Volumes).
* Tools to build dServices and test them.


# Install notes

Download drunner-install:
```
wget https://drunner.s3.amazonaws.com/drunner-install ; chmod a+x drunner-install
```
Install it:
```
drunner-install -v ~/drunner
```

Then you can run drunner.

# Developing dRunner itself

As root:
```
wget -nv -O /tmp/install_docker.sh https://goo.gl/2cxobx ; bash /tmp/install_docker.sh
apt-get install build-essential g++-multilib libboost-all-dev
adduser devuser docker
```

As devuser:
```
git clone git@github.com:drunner/drunner.git
make
```

