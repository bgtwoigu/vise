#!/bin/bash
echo ""
echo "--------------------------------------------------"
echo "Shutting down all docker containers"
echo "(This may take some time)"
echo "--------------------------------------------------"
echo ""

sudo docker stop `sudo docker ps --filter ancestor=registry.gitlab.com/vgg/vise/image:latest -q`