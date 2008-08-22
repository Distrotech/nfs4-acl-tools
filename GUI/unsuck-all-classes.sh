#!/bin/bash
#
# part of turning the ridiculously awful Qt API format into a Javadoc-ish thing
# so it only takes up 1 browser window.
#
# paths will need to be retuned.


qtdocs="/home/richterd/projects/ACL/qt-4.1.4/doc/html"
unsucker="/home/richterd/projects/ACL/tmp/nfs4-acl-2.2.29-3/GUI/unsuck-each-class.sh"

cd $qtdocs

for file in *; do $unsucker $file tmp \; ; done
