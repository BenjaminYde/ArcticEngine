#!/usr/bin/env bash

CURRENT_DIR=$(dirname $0)
REPO_DIR=$(realpath $CURRENT_DIR/../..)

# configure shell rc's:
cd $CURRENT_DIR
for shellrc in .shrc .bashrc .zshrc; do
    cat shellrc >> ~/$shellrc
done

# source shellrc in current shell session
source ./shellrc