zeroprog
========

EEPROM reader/writer for ZEROPLUS LAP-C logic analyzer


What's zeroprog?
----------------

ZEROPLUS LAP-C logic analyzer has EEPROM that is stored USB device information.

zeroprog can read/write/backup/restore the EEPROM.

zeroprog is a part of "zerominus" project (<https://code.google.com/p/zerominus/>).
This version is forked from that project.

This forked version has additional features:

* Change word-endian of backup file for human readable
* Implement 'restore' command

How to build
============

zeroprog requires libusb-1.0.

Install requirements on Ubuntu 14.04:

    $ sudo apt-get install libusb-1.0-0-dev

Checkout and build zeroprog

    $ git clone https://github.com/t-bucchi/zeroprog.git
    $ cd zeroprog
    $ make


How to use
==========

Connect ZEROPLUS LAP-C logic analyzer to your host PC.

zeroprog with --help option shows all options:

    $ sudo ./zeroprog --help
    ./zeroprog 1.0
    Usage: ./zeroprog <command> <argument>
    where <command> is one of the following:
      -v, --vendor	change the vendor ID to the default 0x0c12
      -p, --product <PID>	change the product ID
    	0x700b LAP-C(32128)
    	0x700c LAP-C(321000)
    	0x700d LAP-C(322000)
    	0x700a LAP-C(16128)
    	0x7009 LAP-C(16064)
    	0x700e LAP-C(16032)
    	0x7016 LAP-C(162000)
      -m, --manufacturer <string>	change the manufacturer string
      -o, --model <string>	change the model name string
      -s, --serial <string>	change the serial string
      -b, --backup <filename>	dump the entire EEPROM to file
      -r, --restore <filename>	restore the entire EEPROM from a file
    If no command is given, the current contents of the EEPROM will be displayed.


Read EEPROM and decode contents

    $ sudo ./zeroprog

Backup EEPROM

    $ sudo ./zeroprog -b backupfile.bin

Restore EEPROM

    $ sudo ./zeroprog -r backupfile.bin

Change product ID to LAP-C16128

    $ sudo ./zeroprog -p 0x700a

Change model strings to LAP-C16128

    $ sudo ./zeroprog -o 'ZEROPLUS Logic Analyzer(LAP-C16128)'


License
=======

"zerominus" which is original project is licensed under 3-clause BSD license.

Consequently, zeroprog is also licensed under 3-clause BSD license.
