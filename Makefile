
all :
	echo "Nothing to build, use 'sudo make install' to install faust2spectrogram (requires faust to be installed)"

install :
	cp faust2spectrogram /usr/local/bin/faust2spectrogram
	chmod a+x /usr/local/bin/faust2spectrogram
	cp spectrogram.cpp /usr/local/share/faust/

uninstall :
	rm -f /usr/local/bin/faust2spectrogram
	rm -f /usr/local/share/faust/spectrogram.cpp