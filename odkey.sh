#!/bin/bash
# http://unix.stackexchange.com/questions/110236/how-to-find-out-the-escape-sequence-my-keyboards-sends-to-terminal
{
  stty raw min 1 time 20 -echo
  dd count=1 2> /dev/null | od -vAn -tc | sed -e 's/ \+/ /g' -e 's/^ 033 \[ /CSI /' -e 's/^ 033 O /SS3 /'
  stty sane
}
