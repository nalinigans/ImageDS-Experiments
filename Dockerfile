# The MIT License (MIT)
# Copyright (c) 2019 Nalini Ganapati
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

#ARG os=ubuntu:trusty
ARG os=centos:7
FROM $os

ARG user=docker

COPY scripts/prereqs /build
WORKDIR /build
RUN ./install_prereqs.sh

RUN groupadd -r try_itk && useradd -r -g try_itk -G wheel -m ${user} && echo ${user}:${user} | chpasswd

USER ${user}
WORKDIR /home/${user}
RUN source /opt/rh/devtoolset-7/enable && git clone https://github.com/InsightSoftwareConsortium/ITK.git -b release ITK \
  && cd ITK && mkdir build && cd build && echo $PATH && cmake3 -DCMAKE_INSTALL_PREFIX=~ .. && make && make install

USER ${user}
WORKDIR /home/${user}
ENTRYPOINT ["/bin/bash", "--login"]


