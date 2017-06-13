#!/bin/bash

if [[ $(sudo docker images -q registry.gitlab.com/vgg/vise/image:latest) ]]; then
    echo "--------------------------------------------------"
    echo "VISE Docker image is already loaded."
    echo "Recall that you have to run the load script only once"
    echo "--------------------------------------------------"
else 
    echo ""
    echo "--------------------------------------------------"
    echo "Loading VISE docker image ..."
    echo "(Note: this may take some time)"
    echo "--------------------------------------------------"
    echo ""
    SCRIPT_DIR=$(dirname $0)

    sudo docker load --input "${SCRIPT_DIR}/vise-1.0.0.tar" --quiet
fi
