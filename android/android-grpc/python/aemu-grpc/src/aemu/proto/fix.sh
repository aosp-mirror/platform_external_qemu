#!/bin/bash

for link in $(find . -type l); do
    target=$(readlink $link)
    new_target=../../../../../services/$(basename $link)
    echo "Updating $link -> $new_target"
    unlink $link
    ln -s $new_target $link
    git add $link
        #mv $link $new_target
        #ln -s services/$new_target $link
done

