#!/bin/bash

set -eux

git add *
git commit -m 'new commit' || true
git push
mkdocs build
rm -Rf /tmp/site || true
mv site /tmp/site
git checkout master
rsync -av /tmp/site/* .
rm -Rf /tmp/site
git add *
git commit -m 'new commit'
git push
git checkout source
