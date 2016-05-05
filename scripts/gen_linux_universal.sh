#!/bin/sh
set -e

for f in `find builds/linux* -name "bin" -type d`; do
cat << EOF > $f/amulet.sh
#!/bin/sh
exec \$0.`uname -m` \$@
EOF
chmod +x $f/amulet.sh
done
