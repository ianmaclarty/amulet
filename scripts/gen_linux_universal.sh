#!/bin/sh
set -e

for f in `find builds/linux* -name "bin" -type d`; do
cat << EOF > $f/amulet.sh
#!/bin/sh
LD_LIBRARY_PATH="\$LD_LIBRARY_PATH:lib.\`uname -m\`" exec \$0.\`uname -m\` \$@
EOF
chmod +x $f/amulet.sh
done
