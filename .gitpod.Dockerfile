FROM gitpod/workspace-full-vnc
USER gitpod
RUN git clone https://github.com/akheron/jansson.git && \
    cd jansson && \
    autoreconf -fi  && \
    ./configure && \
    make all -j && \
    sudo make install && \
    cd .. && rm -rf jansson
    
RUN git clone https://github.com/benmcollins/libjwt.git  && \
    cd libjwt && \
    autoreconf --install && \
    ./configure && \
    make all -j && \
    sudo make install && cd .. && rm -rf libjwt
    
RUN wget https://curl.se/download/curl-7.76.0.tar.gz  && \
    tar -xvf curl-7.76.0.tar.gz && \
    cd curl-7.76.0 && \
    ./configure && \
    make -j  && \
    sudo make install && cd .. && rm -rf curl-7.76.0*
RUN wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.72.tar.gz && \
    tar -xvf libmicrohttpd-0.9.72.tar.gz && \
    cd libmicrohttpd-0.9.72 && mkdir build && cd build && ../configure && \
    sudo make install && cd .. && rm -rf libmicrohttpd-0.9.72*
