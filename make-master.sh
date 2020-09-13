#!/bin/bash

git add *
git commit -m 'new commit'
git push
mkdocs build
rm -Rf /tmp/site
mv site /tmp/site
git checkout master
mv /tmp/site/* .
git add *
git commit -m 'new commit'
git push
git checkout source
