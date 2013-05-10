require 'erb'

# Debian package control file.
CONTROL_TPL = ERB.new <<-EOF
Package: <%= name %>
Version: <%= version %>
Section: base
Priority: optional
Architecture: <%= architecture %>
Depends: libasound2 (>= 1.0.25)
Maintainer: Benny Bach <benny.bach@gmail.com>
Description: SpotiHifi daemon
EOF

# /usr/bin/spotihifid
LAUNCH = <<eos
#!/bin/bash
INSTDIR=/usr/share/spotihifi
LD_LIBRARY_PATH+=$INSTDIR/lib/ $INSTDIR/bin/spotihifid -c /etc/spotihifi.conf
eos

PKG_DIRS = %w(debian/DEBIAN
              debian/usr/bin
              debian/usr/share/spotihifi/bin
              debian/usr/share/spotihifi/lib)

desc "Build debian package"
task :dpkg do

  # Create directory structure.
  PKG_DIRS.each do |dir|
    sh "mkdir -p #{dir}"
  end

  name = "spotihifi"
  version = "0.0-1"
  architecture = %x[dpkg --print-architecture].strip

  # Create debian control file.
  File.open("debian/DEBIAN/control", "w") do |file|
    file.write(CONTROL_TPL.result(binding))
  end

  # Create spotihifid launch script.
  File.open("debian/usr/bin/spotihifid", "w") do |file|
    file.write(LAUNCH)
  end
  sh "chmod +x debian/usr/bin/spotihifid"

  # Copy binaries.
  sh "cp build/spotihifid debian/usr/share/spotihifi/bin/"
  sh "cp #{Dir['vendor/libspotify*'].first}/lib/libspotify.* debian/usr/share/spotihifi/lib/"

  # Build package.
  sh "dpkg-deb --build debian"

  # Rename debian.dep
  sh "mv debian.deb #{name}_#{version}-#{architecture}.deb"

end
